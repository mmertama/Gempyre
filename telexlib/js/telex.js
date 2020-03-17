/*jshint esversion: 6 */
var uri = "ws://" +  window.location.hostname + ':' + window.location.port + "/telex";
//var uri = "ws://127.0.0.1:8080/telex";
var socket = new WebSocket(uri);
socket.binaryType = 'arraybuffer';

var logging = true;

function log(...logStr) {
    if(logging)
        console.log(...logStr);
} 

function errlog(source, text) {
    source = source || "Unknown";
    console.error("error:" + source + " --> " + text);
    socket.send(JSON.stringify({'type': 'error', 'element': String(source), 'error': text}));
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
    if(el.id != id) {
        const element = document.getElementById(id);
        if(!element) {
            errlog("removeElement", "Cannot find " + id);
            return;
        }
        if(element.parentNone == el)
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
  const lastFuncion = fn;
  return function (...args) {
    const now = (new Date).getTime();
    if (lastCall && now - lastCall < delay) {
      //  setTimeout(function() {
      //      if(lastFuncion)
      //          lastFuncion.apply(this, args);
      //      lastFuncion = null;
      //  }, delay + 1);
        return;
    }
    lastCall = now;
    const rvalue = lastFuncion(...args);
    lastFuncion = null;
    return rvalue;
  }
}

function addEvent(el, source, eventname, properties, throttle) {
    console.log("addEventing", el, source, eventname, throttle);
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
    el.addEventListener(eventname, throttle && throttle > 0 ? throttled(throttle, handler) : handler);
}

function sendTelexEvent(source, eventname, values) {
    if(typeof source !== "string" || typeof eventname !== "string" || typeof values !== "object") {
        console.assert(typeof source === "string", "source should be string");
        console.assert(typeof eventname === "string", "eventname should be string");
        console.assert(typeof values === "object", "values should be object");
        return false;
    }
    if(!socket) {
         console.error("No socket");
        return false;
    }
    log("do Telex event", source, eventname, values);
    socket.send(JSON.stringify({'type': 'event',  'element': source, 'event': eventname, 'properties':values}));
    return true;
}

function id(el) {
    console.assert(el.nodeType == 1, "Shall not get id of non element");
    if(!el.id)
        el.id = "telex_"+ Math.random().toString(32).substr(2,16);
    return el.id;
}

function serveQuery(element, query_id, query) {
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
                if(c.nodeType == 1) //only elements
                    children.push(id(c));  //just ids
            }
            socket.send(JSON.stringify({'type': 'query', 'query_id': query_id, 'query_value': 'children', 'children': children}));
            break;
        case 'value':
            socket.send(JSON.stringify({
                'type': 'query',
                'query_id': query_id,
                'query_value': 'value',
                'value': {value: el.value, checked: el.checked, 'name': el.name, 'named':el[el.name]}}));
            break;
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
            socket.send(JSON.stringify({
               'type': 'query',
               'query_id': query_id,
               'query_value': 'bounding_rect',
               'bounding_rect': {'x':r.left, 'y':r.top, 'width': r.right - r.left, 'height': r.bottom - r.top}}));
           break;
        default:
            errlog(query_id, "Unknown query " + query);
    }
}

function sendCollection(name, query_id, query, collectionFunction) {
    const children = [];
    const collection = collectionFunction(name);
    for(const c of collection) {
        if(c.nodeType == 1)
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
        let dataOffset = 4 * 4; //id, datalen, idlen, headerlen, data<datalen>, header<headerlen>, id<idlen>
        const data = new Uint8ClampedArray(buffer, dataOffset, datalen);
        const headerOffset = bytes[1] + 4;
        const x = bytes[headerOffset];
        const y = bytes[headerOffset + 1];
        const w = bytes[headerOffset + 2];
        const h = bytes[headerOffset + 3];
        const idOffset = (4 * 4) + dataOffset + datalen;
        console.log("Datam", bytes, dataOffset, x, y, w, h, idOffset);
        const words = new Uint16Array(buffer, idOffset, idLen * 2);
        let id = "";
        for(let i = 0 ; i < words.length && words[i] > 0; i++)
            id += String.fromCharCode(words[i]);
        const element = document.getElementById(id);
        if(!element) {
            errlog(id, "element not found");
            return;
        }
        const ctx = element.getContext("2d", {alpha:false});
        if(ctx) {
            const bytesLen = w * h * 4;
            const imageData = data.length == bytesLen ? new ImageData(data, w, h) : new ImageData(data.slice(0, bytesLen), w, h);
            ctx.putImageData(imageData, x, y);
        } else {
            errlog(id, "has no graphics context");
            return;
        }
    } else {
        errlog("Unknown", "Unknown binary message type", bytes);
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

function handleJson(msg) {
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
            logging = msg.logging == "true" ? true : false;
            return;
        case 'debug':
            console.log(msg.debug);
            return;
        case 'alert':
            alert(msg.alert);
            return;
        case 'eval':
            eval(msg.eval);
            return;
        case 'open':
            window.open(msg.url, msg.view.lenght > 0 ? msg.view : '_blank');
            return;
        case 'create':
            if(msg.element == undefined || msg.element.length == 0) {
                createElement(null, msg.html_element, msg.new_id);
                return;
            } break;
        case 'query':
            switch(msg.query) {
            case 'exists':
                socket.send(JSON.stringify({'type': 'query', 'query_id': msg.query_id, 'query_value': 'exists', 'exists': document.getElementById(msg.id) != null}));
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
        }

        if(msg.type == 'query') {
            serveQuery(msg.element, msg.query_id, msg.query);
            return;
        }
    
        const el = msg.element.length > 0 ?  document.getElementById(msg.element) : document.body;
        if(!el) {
            errlog(msg.element, 'not found:"', msg.element, '"');
            return;
        }
        switch(msg.type) {
            case 'html':
                el.innerHTML = msg.html;
                break;
            case 'attribute':
                el.setAttribute(msg.attribute,  msg.value);
                break;
            case 'create':
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
    socket.send(JSON.stringify({'type': 'uiready'}));
};


socket.onmessage =
        function(event) {
    if(event.data instanceof ArrayBuffer) {
        handleBinary(event.data);
        return;
    }
    //let msg = null;
    //try {
    const msg = JSON.parse(event.data);
    //}  catch(error) {
    //      errlog("onMessage - exception:", error + " on Event event:" + event.data);
    //}
    handleJson(msg);
 }

socket.onerror = function(event) {
    console.error(event);
    window.open('','_self').close(); //we may close
};

socket.onclose = function(event) {
    log(event);
    window.open('','_self').close(); //we may close
};




