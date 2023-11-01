/*jshint esversion: 6 */
var gempyreAddress = window.location.hostname + ':' + window.location.port;
var httpUrl = "http://" + gempyreAddress;
var uri = "ws://" + gempyreAddress + "/gempyre";

//var uri = "ws://127.0.0.1:8080/gempyre";
var socket = new WebSocket(uri);
socket.binaryType = 'arraybuffer';

var logging = false;

var sys_log = console.log;
var sys_warn = console.warn;
var sys_info = console.info;
var sys_error = console.error;

const event_notifiers = new Set(); // For non-JS nottifiers

var last_msg_id = -1;

function g_log(msg) {
    const logged = Array.prototype.slice.call(arguments).join(', ');
    socket.send(JSON.stringify({'type': 'log', 'level': 'log', 'msg': logged}));
    sys_log(msg);
}

function g_warn(msg) {
    const logged = Array.prototype.slice.call(arguments).join(', ');
    socket.send(JSON.stringify({'type': 'log', 'level': 'warn', 'msg': logged}));
    sys_warn(msg);
}

function g_info(msg) {
    const logged = Array.prototype.slice.call(arguments).join(', ');
    socket.send(JSON.stringify({'type': 'log', 'level': 'info', 'msg': logged}));
    sys_info(msg);
}

function g_error(msg) {
    const getTrace = function() {
      const obj = {};
      if (Error.captureStackTrace !== undefined) {
        Error.captureStackTrace(obj, getTrace);
      }
      return obj.stack;
    };
    const logged = Array.prototype.slice.call(arguments).join(', ');
    socket.send(JSON.stringify({'type': 'log', 'level': 'error', 'msg': logged, 'trace': getTrace()}));
    sys_error(msg);
}


function log(...logStr) {
    if(logging) {
        console.log(...logStr);
    }
} 

function errlog(source, text) {
    source = source || "Unknown";

    const getTrace = function() {
      const obj = {};
      if (Error.captureStackTrace !== undefined) {
        Error.captureStackTrace(obj, getTrace);
      }
      return obj.stack;
    };

    console.error("error:" + source + " --> " + text);
    socket.send(JSON.stringify({'type': 'error', 'element': String(source), 'error': text, 'trace': getTrace()}));
}

function catchLog(error, extra) {
    source = error.name || "Unknown";
    console.error("error:" + source + " --> " + error.message + " \"" + (extra ? extra : '"'));
    socket.send(JSON.stringify({'type': 'error',
        'element': String(source),
        'error': error.message + " \"" + (extra ? extra : '"'), 'trace': error.stack}));
}

function assert(condition, msg) {
    if (!condition) {
        throw Error('assert: ' +  condition + " " + (msg || '""'));
    }
}

function createElement(parent, tag, id) {
    
    if(document.getElementById(id) == null) {
        const el = document.createElement(tag);
        if(!el) {
            errlog("createElement", "Cannot create " + id);
            return;
        }
        el.setAttribute('id', id);
        if(parent)
            parent.appendChild(el);
        else
            document.body.appendChild(el);
        log("created", tag, id, parent, el.id);
    } else {
        errlog("createElement", "Element exists " + id);
    }
}

function removeElement(el, id) {
    if(el.id !== id) {
        const element = document.getElementById(id);
        if(!element) {
            errlog("removeElement", "Cannot find " + id);
            return;
        }
        if(element.parentNone === el)
            el.removeChild(element);
        else {
            errlog("removeElement", el.id + " is not a parent of " + id);
            return;
        }
    } else {
        el.parentNode.removeChild(el);
    }
}


function throttled(delay, fn) {
  let lastCall = null;
  const lastFunction = fn;
  return function (...args) {
    const now = (new Date).getTime();
    if (lastCall && now - lastCall < delay) {
        return;
    }
    lastCall = now;
    const rvalue = lastFunction(...args);
   // lastFunction = null;
    return rvalue;
  }
}

function addEvent(el, source, eventname, properties, throttle) {
    handler = (event) => {

        if(socket.readyState !== 1)
            return;

        if(socket.bufferedAmount > 0) {
            if(eventname === "mousemove") {
                return;
            }
        }

        event.stopPropagation();

        const values = {};
        for(const key of properties) {
            if(key in event) {
                values[key] = event[key];
            }
            else if(event.currentTarget && key in event.currentTarget) {
                values[key] = event.currentTarget[key];
            }
            else if(event.target && key in event.target) {
                values[key] = event.target[key];
            }
        }

        log("do event", el, source, eventname, values, event);
        socket.send(JSON.stringify({'type': 'event',  'element': source, 'event': eventname, 'properties':values}));
    };

    const usedHandler = throttle && throttle > 0 ? throttled(throttle, handler) : handler;
    if(eventname === 'resize') { //Only window supports the resize event
        log("addEventing", "window", source, eventname, throttle);
        window.addEventListener(eventname, usedHandler);
    }
    else if(el === document.body && eventname === 'scroll' ) { //root == document.body, document listens scroll
        log("addEventing", "document", source, eventname, throttle);
        document.addEventListener(eventname, usedHandler);
    }
    else if(eventname === 'load') {
        let on_complete = function() {
            if(properties.includes('complete') && (!el.complete || (0 == el.width && 0 == el.height))) {
                log("Element ", el.id, "is not ready....wait");
                setTimeout(on_complete, 500);
            } else {
                log("Element ", el.id, "is ready...send", el.complete, el.width, el.height);
                socket.send(JSON.stringify({'type': 'event',  'element': el.id, 'event': 'load', 'properties': {
                    'complete': el.complete,
                    'width': el.width,
                    'height': el.height
                }}));
            }
        };
        if (el.complete) {
            on_complete();
        } else {
            log("addEventing", el, source, eventname, throttle);
             // addEventListener(eventname, usedHandler); works only with window
            el.onload = on_complete;
            }
    }
    else {
        log("addEventing", el, source, eventname, throttle);
        el.addEventListener(eventname, usedHandler);
    }
}

function sendGempyreEvent(source, eventname, values) {
    if(typeof source !== "string" || typeof eventname !== "string" || typeof values !== "object") {
        assert(typeof source === "string", "source should be string");
        assert(typeof eventname === "string", "eventname should be string");
        assert(typeof values === "object", "values should be object");
        return false;
    }
    if(!socket) {
         console.error("No socket");
        return false;
    }
    log("do Gempyre event", source, eventname, values);
    socket.send(JSON.stringify({'type': 'event',  'element': source, 'event': eventname, 'properties':values}));
    return true;
}

id_inc = 0;

function id(el) {
    assert(el.nodeType === 1, "Shall not get id of non element");
    if(!el.id) {
        el.id = 'gempyre_' + Math.random().toString(32).substring(2, 2 + 16) + '_' + (++id_inc).toString(32);
    }
    return el.id;
}

function serveQuery(element, query_id, query, query_params) {
    const el = element.length > 0 ? document.getElementById(element) : document.body;
    if(!el) {
        errlog(element, 'not found:', element, '" for query"');
        socket.send(JSON.stringify({'type': 'query', 'query_id': query_id, 'query_value':'query_error', 'query_error':'query_error'}));
        return;
    }
    log("query", el, query_id, query);
    switch(query) {
        case 'attributes':
            const attributes = new Object();
            for(const a of el.attributes) //attributes is NamedNodeMap not on list of pairs
                attributes[a.name] = a.value;
            socket.send(JSON.stringify({'type': 'query', 'query_id': query_id, 'query_value': 'attributes', 'attributes': attributes}));
            break;
         case 'children':
            const children = [];
            for(const c of el.childNodes) {
                if(c.nodeType === 1) //only elements
                    children.push(id(c));  //just ids
            }
            socket.send(JSON.stringify({'type': 'query', 'query_id': query_id, 'query_value': 'children', 'children': children}));
            break;
        case 'parent':
            // ids are not allowed to have spaces, so we use that abonomination for a root :-D
            let parentValue = (
                el.parentNode == document.body ||
                el.parentElement == document.body) ? ": :" : el.parentElement.id;
            socket.send(JSON.stringify({
                'type': 'query',
                'query_id': query_id,
                'query_value': 'parent',
                'parent': parentValue})); 
            break;    
        case 'value':
            socket.send(JSON.stringify({
                'type': 'query',
                'query_id': query_id,
                'query_value': 'value',
                'value': {value: el.value, checked: el.checked, 'name': el.name, 'named':el[el.name]}}));
            break;
        case 'styles':
            const styles = new Object();
            const computedStyles = window.getComputedStyle(el);
            for(const s of query_params)
                styles[s] = computedStyles[s]
            //    if(s in computedStyles)
            //    styles[s.name] = s.value;
            socket.send(JSON.stringify({
                'type': 'query',
                'query_id': query_id,
                'query_value': 'styles',
                'styles': styles
              //  'styles': {'obj':'styles', 'type': typeof(computedStyles), 'sz':Object.keys(computedStyles)}
                                       }));
            break
        case 'innerHTML':
             socket.send(JSON.stringify({
                'type': 'query',
                'query_id': query_id,
                'query_value': 'innerHTML',
                'innerHTML': el.innerHTML}));
            break;
        case 'element_type':
            socket.send(JSON.stringify({
               'type': 'query',
               'query_id': query_id,
               'query_value': 'element_type',
               'element_type': el.nodeName.toLowerCase()}));
           break;
        case 'bounding_rect':
            const r = el.getBoundingClientRect();
           // const r = (el != document.root) ? el.getBoundingClientRect() : function() {
           //     return {'left':0,'top':0,'right': window.outterWidth,'bottom': window.outterHeight + 400};
           // }();
            socket.send(JSON.stringify({
               'type': 'query',
               'query_id': query_id,
               'query_value': 'bounding_rect',
               'bounding_rect': {'x':r.left, 'y':r.top, 'width': r.right - r.left, 'height': r.bottom - r.top}}));
           break;
        case 'devicePixelRatio':
            socket.send(JSON.stringify({
                                           'type': 'query',
                                           'query_id': query_id,
                                           'query_value': 'devicePixelRatio',
                                           'devicePixelRatio': window.devicePixelRatio
                                       }));
            break;
        default:
            errlog(query_id, "Unknown query " + query);
    }
}

function sendCollection(name, query_id, query, collectionFunction) {
    const children = [];
    const collection = collectionFunction(name);
    for(const c of collection) {
        if(c.nodeType === 1)
            children.push(id(c)); 
    }
    socket.send(JSON.stringify({'type': 'query', 'query_id': query_id, 'query_value': 'children', 'children': children}));   
}

function handleBinary(buffer) {
    const bytes = new Uint32Array(buffer);
    if(!bytes || bytes.length === 0) {
        errlog("Binary", "Invalid buffer", buffer);
        return;
    }
    const type = bytes[0];
    if(type === 0xAAA) {
        const datalen = bytes[1] * 4;
        const idLen = bytes[2];
        const headerLen = bytes[3];
        let dataOffset = 4 * 4; //id, datalen, idlen, headerlen, data<datalen>, header<headerlen>, id<idlen>
        const data = new Uint8ClampedArray(buffer, dataOffset, datalen);
        const headerOffset = bytes[1] + 4;
        const x = bytes[headerOffset];
        const y = bytes[headerOffset + 1];
        const w = bytes[headerOffset + 2];
        const h = bytes[headerOffset + 3];
        const as_draw = bytes[headerOffset + 4];
        const idOffset = (5 * 4) + dataOffset + datalen;
        const words = new Uint16Array(buffer, idOffset, idLen);
        let id = "";
        for(let i = 0 ; i < words.length && words[i] > 0; i++)
            id += String.fromCharCode(words[i]);
        const element = document.getElementById(id);

        if(!element) {
            errlog(id, "Canvas not found '" + id + "'" + " at: ", idOffset, " len: ", idLen);
            return;
        }

        if (datalen > 0) { // otherwise this is just a tail
            const ctx = element.getContext("2d", {alpha:false});
            if(ctx) {
                if(w > 0 && h > 0) {
                    const bytesLen = w * h * 4;
                    const imageData = data.length === bytesLen ? new ImageData(data, w, h) : new ImageData(data.slice(0, bytesLen), w, h);
                    ctx.putImageData(imageData, x, y);
                }
            } else {
                errlog(id, "has no graphics context");
                return;
            }
        }    


        // if as_draw AND there is a notification request - send a notify
        if ((as_draw != 0) && event_notifiers.has("canvas_draw")) {
            socket.send(JSON.stringify({
                                            'type': 'event',
                                            'element': id,
                                            'event': 'event_notify',
                                            'properties':{
                                                'name': "canvas_draw",
                                                'msgid': 0
                                            }
                                        }));
            
        }

    } else {
        errlog("Unknown", "Unknown binary message type: " + type.toString(16), bytes);
    }
}

function paintImage(element, imageName, pos, rect, clip) {
    const image = document.getElementById(imageName);
    if(!image) {
        errlog(imageName, "not found on paint");
        return;
    }
    const ctx = element.getContext("2d");
    if(!ctx) {
        errlog(id, "has no graphics context");
        return;
    }
    if(pos) {
        if(clip)
            ctx.drawImage(image, clip[0], clip[1], clip[2], clip[3], pos[0], pos[1], image.width, image.height);
        else
            ctx.drawImage(image, pos[0], pos[1]);
    } else if(rect) {
        if(clip)
            ctx.drawImage(image, clip[0], clip[1], clip[2], clip[3], rect[0], rect[1], rect[2], rect[3]);
        else
            ctx.drawImage(image,  rect[0], rect[1], rect[2], rect[3]);
    } else {
        errlog(id, "image has not position");
        return;
    }
}

function canvasDraw(element, commands) {
    const ctx = element.getContext("2d");
    if(!ctx) {
        errlog(id, "has no graphics context");
        return;
    }
    let cmdpos = 0;
    while(cmdpos < commands.length) {
        const cmd = commands[cmdpos++];
        switch(cmd) {
        case 'strokeRect':
            ctx.strokeRect(commands[cmdpos++], commands[cmdpos++], commands[cmdpos++], commands[cmdpos++]);
            break;
        case 'clearRect':
            ctx.clearRect(commands[cmdpos++], commands[cmdpos++], commands[cmdpos++], commands[cmdpos++]);
            break;
        case 'fillRect':
            ctx.fillRect(commands[cmdpos++], commands[cmdpos++], commands[cmdpos++], commands[cmdpos++]);
            break;
        case 'fillText':
            ctx.fillText(commands[cmdpos++], commands[cmdpos++], commands[cmdpos++]);
            break;
        case 'strokeText':
            ctx.strokeText(commands[cmdpos++], commands[cmdpos++], commands[cmdpos++]);
            break;
        case 'arc':
            ctx.arc(commands[cmdpos++], commands[cmdpos++], commands[cmdpos++], commands[cmdpos++], commands[cmdpos++]);
            break;
        case 'ellipse':
            ctx.ellipse(commands[cmdpos++], commands[cmdpos++], commands[cmdpos++], commands[cmdpos++], commands[cmdpos++], commands[cmdpos++], commands[cmdpos++]);
            break;
        case 'beginPath':
            ctx.beginPath();
            break;
        case 'closePath':
            ctx.closePath();
            break;
        case 'lineTo':
            ctx.lineTo(commands[cmdpos++], commands[cmdpos++]);
            break;
        case 'moveTo':
            ctx.moveTo(commands[cmdpos++], commands[cmdpos++]);
            break;
        case 'bezierCurveTo':
             ctx.bezierCurveTo(commands[cmdpos++], commands[cmdpos++], commands[cmdpos++], commands[cmdpos++], commands[cmdpos++], commands[cmdpos++]);
             break;
        case 'quadraticCurveTo':
             ctx.quadraticCurveTo(commands[cmdpos++], commands[cmdpos++], commands[cmdpos++], commands[cmdpos++]);
             break;
        case 'arcTo':
             ctx.arcTo(commands[cmdpos++], commands[cmdpos++], commands[cmdpos++], commands[cmdpos++], commands[cmdpos++]);
             break;
        case 'rect':
            ctx.rect(commands[cmdpos++], commands[cmdpos++], commands[cmdpos++], commands[cmdpos++]);
            break;
        case 'stroke':
            ctx.stroke();
            break;
        case 'fill':
            ctx.fill();
            break;
        case 'fillStyle':
            ctx.fillStyle = commands[cmdpos++];
            break;
        case 'strokeStyle':
            ctx.strokeStyle = commands[cmdpos++];
            break;
        case 'lineWidth':
            ctx.lineWidth = commands[cmdpos++];
            break;
        case 'font':
            ctx.font = commands[cmdpos++];
            break;
        case 'textAlign':
            ctx.textAlign = commands[cmdpos++];
            break;
        case 'save':
            ctx.save();
            break;
        case 'restore':
            ctx.restore();
            break;
        case 'rotate':
            ctx.rotate(commands[cmdpos++]);
            break;
        case 'translate':
            ctx.translate(commands[cmdpos++], commands[cmdpos++]);
            break;
        case 'scale':
            ctx.scale(commands[cmdpos++], commands[cmdpos++]);
            break;
        case 'drawImage':
            const image = document.getElementById(commands[cmdpos++]);
            if(!image) {
                errlog("drawImage", commands[cmdpos - 1] + " image not found");
                return;
            }
            ctx.drawImage(image, commands[cmdpos++], commands[cmdpos++]);
            break;
        case 'drawImageRect':
            const image1 = document.getElementById(commands[cmdpos++]);
            if(!image1) {
                errlog("drawImageRect", commands[cmdpos - 1] + " image not found");
                return;
            }
            ctx.drawImage(image1,
                          commands[cmdpos++],
                          commands[cmdpos++],
                          commands[cmdpos++],
                          commands[cmdpos++]);
            break;
        case 'drawImageClip':
            const image2 = document.getElementById(commands[cmdpos++]);
            if(!image2) {
                errlog("drawImageClip", commands[cmdpos - 1] + " image not found");
                return;
            }
            ctx.drawImage(image2,
                          commands[cmdpos++],
                          commands[cmdpos++],
                          commands[cmdpos++],
                          commands[cmdpos++],

                          commands[cmdpos++],
                          commands[cmdpos++],
                          commands[cmdpos++],
                          commands[cmdpos++]);
            break;
        case 'textBaseline':
            ctx.textBaseline = commands[cmdpos++];
            break;
        case 'reset':
            ctx.reset();
            break;        
        default:
            errlog(cmd, "is not supported command:" + cmdpos + ", in commands:" + commands);
            return;
        }
    }
}

function httpGetBin(msg) {
    const pull_id = msg['id'];
    fetch(httpUrl  + '/data/' + pull_id)
    .then(response => response.arrayBuffer())
    .then(arrayBuffer => handleBinary(arrayBuffer))
}

function httpGetJson(msg) {
    const pull_id = msg['id'];
    fetch(httpUrl + '/data/' + pull_id)
    .then(response => response.json())
    .then(json => handleJson(json))
}

function handleJson(msg) {

    if('msgid' in msg) {
        msgid = parseInt(msg.msgid);
        if(msgid <= last_msg_id)
            return;
        last_msg_id = msgid;
    }

    handleJsonCommand(msg);

    if(event_notifiers.has(msg.type)) {
        socket.send(JSON.stringify({
                                       'type': 'event',
                                       'element': msg.element.length ? msg.element : "",
                                       'event': 'event_notify',
                                       'properties':{
                                           'name': msg.type,
                                           'msgid': 'msgid' in msg ? msg.msgid : 0
                                       }}));
    }
}

function setAttribute(el, attribute, value) {
    let val = 0
    try  {
    val = JSON.parse(value);        // In case of number or boolean this works, but string gives Syntax error,
    } catch(ex) {                       // There can be funny side effects with this....
        val = value
    }
    const existing_type = typeof el[attribute];
    const new_type =  typeof val;

    log("setAttribute", existing_type, "as", new_type, "to", val, typeof value);

    if(existing_type != new_type) {
        if(existing_type =='boolean' && new_type == 'string')
            val = val == 'true';
        else if(existing_type =='boolean' && new_type == 'number')
            val = val != 0;
        else if(existing_type =='number' && new_type == 'boolean')
            val = val ? 1 : 0;
        else if(existing_type =='number' && new_type == 'string')
            val = Number(new_type)
        else if(existing_type =='string' && new_type == 'boolean')
            val = val ? "true" : "false";
        else if(existing_type =='string' && new_type == 'number')
            val = String(new_type)            
        }
    el.setAttribute(attribute,  val); //This works in some cases
    el[attribute] = val;              //...and this in some :-D
}

function handleJsonCommand(msg) {
        switch(msg.type) {
        case 'batch':
            msg.batches.forEach(item => handleJson(item));
            return;
        case 'extension':
            return;
        case 'nil':
            return;
        case 'exit_request':
            socket.send(JSON.stringify({'type': 'exit_request'}));
            log("Bye bye");
            socket.close();
            return;
        case 'close_request':
            log("Good bye");
            socket.close();
            return;
        case 'logging':
            logging = msg.logging === "true" ? true : false;
            if(logging) {
                console.log = g_log;
                console.info = g_info;
                console.warn = g_warn;
                console.error = g_error;
            } else {
                console.log = sys_log;
                console.info = sys_info;
                console.warn = sys_warn;
                console.error = sys_error;
            }
            return;
        case 'debug':
            console.log(msg.debug);
            return;
        case 'alert':
            alert(msg.alert);
            return;
        case 'eval':
            try {
                eval(msg.eval);
            } catch(ex) {
                errlog(msg.eval, ex.toString());
            }
            return;
        case 'open':
            ///socket.close(); // close current if new (or reload) establish a new connection
            window.open(msg.url, msg.view && msg.view.length > 0 ? msg.view : '_blank');
            return;
        case 'create':
            if(msg.element == undefined || msg.element.length == 0) {
                createElement(null, msg.html_element, msg.new_id);
                return;
            }
            break;
        case 'query':
            switch(msg.query) {
            case 'exists':
                socket.send(JSON.stringify({'type': 'query', 'query_id': msg.query_id, 'query_value': 'exists', 'exists': msg.element == "" || document.getElementById(msg.element) != null}));
                return;
            case 'classes':
                sendCollection(msg.element, msg.query_id, msg.query, function(name) {return document.getElementsByClassName(name);});
                return;
            case 'names':
                sendCollection(msg.element, msg.query_id, msg.query, function(name){return document.getElementsByName(name);});
                return;
            case 'ping':
                socket.send(JSON.stringify({'type': 'query', 'query_id': msg.query_id, 'query_value': 'pong', 'pong': String(Date.now())    }));
                return;
            } break;
        case 'pull_binary':
            httpGetBin(msg);
            return;
        case 'pull_json':
            httpGetJson(msg);
            return;
        case 'event_notify':
            if(msg.add)
                event_notifiers.add(msg.name);
            else
                event_notifiers.delete(msg.name);
            return;
        }

        if(msg.type === 'query') {
            serveQuery(msg.element, msg.query_id, msg.query, msg.query_params);
            return;
        }
    
        const el = msg.element.length > 0 ?  document.getElementById(msg.element) : document.body;
        if(!el) {
            errlog(msg.element, 'element not found:"' + msg.element + '"');
            return;
        }
        switch(msg.type) {
            case 'html':
                el.innerHTML = msg.html;
                break;
            case 'set_attribute':
                setAttribute(el, msg.attribute, msg.value);
                break;
            case 'create':
                log("create", el, msg.html_element, msg.new_id);
                createElement(el, msg.html_element, msg.new_id);
                break;
            case 'remove':
                removeElement(el, msg.remove);
                break;
            case 'event':
                addEvent(el, msg.element, msg.event, msg.properties, msg.throttle);
                break;
            case 'paint_image':
                paintImage(el, msg.image, msg.pos, msg.rect, msg.clip);
                break;
            case 'canvas_draw':
                canvasDraw(el, msg.commands);
                break;
            case 'remove_attribute':
                el.removeAttribute(msg.attribute)
                break;
            case 'set_style':
                el.style[msg.style] = msg.value;
                break;
            case 'remove_style':
                el.style.removeProperty(msg.style);
                break;
            case 'tag_name':
                sendCollection(msg.element, msg.query_id, msg.query, function(name){return el.getElementsByTagName(name);});
                return;        
            default:
                errlog(msg.type, "Unknown type");       
        }
};


socket.onopen = function(event) {
    log("onopen", uri, event);
    setInterval(function() {
        if(socket.readyState === 1)
            socket.send(JSON.stringify({'type': 'keepalive'}));
    }, 10000); //decreased to help more intensive cal app messages (read mandelbrot) get passed
    socket.send(JSON.stringify({'type': 'ui_ready'}));
};

socket.onmessage =
        function(event) {
        try {        
            if(event.data instanceof ArrayBuffer) {
                handleBinary(event.data);
                return;
            }
            const msg = JSON.parse(event.data);
            handleJson(msg);
        } catch(error) {
            catchLog(error, event.data);
        }
 }

socket.onerror = function(event) {
    console.error(event);
    window.open('','_self').close(); //we may close
};

socket.onclose = function(event) {
    log(event);
    window.open('','_self').close(); //we may close
};




