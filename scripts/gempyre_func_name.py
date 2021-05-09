import sys
import re
import glob
import html
import Gempyre
from Gempyre_utils import resource


def parse_py11_cpp(p):
    in_block = 0
    mod = None

    tree = {}
    root = tree
    parent = None

    def class_type_regex(name):
        cr = r'py::' + name + r'_<(.+)>\(\s*(\w+)\s*\,\s*"(\w+)"\s*\)'
        return cr

    def value_type_regex(name):
        rr = r'^\s*(' + mod + r')?\.' + name + r'\(\s*"(\w+)"\s*\,\s*\&?((:|\w)+)\s*(\)|,)'
        return rr

    for line in p:
        m = re.search(r'PYBIND11_MODULE\(\s*(\w+)\s*\,\s*(\w+)\s*\)', line)
        if m:
            module_name = m[1]
            mod = m[2]
            tree[module_name] = {'__block': in_block,
                                 '__type': 'module',
                                 '__methods': []}
            parent = tree
            tree = tree[module_name]

        m = re.search(class_type_regex('class'), line)
        if m:
            class_name = m[1]
            mod = m[2]
            tree[class_name] = {'__block': in_block,
                                '__type': 'class',
                                '__py_name': m[3],
                                '__values': [],
                                '__methods': [],
                                '__readonly': [],
                                '__static': []}
            parent = tree
            tree = tree[class_name]

        m = re.search(class_type_regex('enum'), line)
        if m:
            enum_name = m[1]
            mod = m[2]
            tree[enum_name] = {'__block': in_block,
                               '__type': 'enum',
                               '__py_name': m[3],
                               '__enums': []}
            parent = tree
            tree = tree[enum_name]

        def parse_values():
            m = re.search(value_type_regex('value'), line)
            if m:
                assert (tree['__type'] == 'enum')
                tree['__enums'].append((m[2], m[3]))
                return
            m = re.search(value_type_regex('def'), line)
            if m:
                assert (tree['__type'] == 'class' or tree['__type'] == 'module')
                tree['__methods'].append((m[2], m[3]))
                return
            m = re.search(value_type_regex('def_readwrite'), line)
            if m:
                assert (tree['__type'] == 'class')
                tree['__values'].append((m[2], m[3]))
                return
            m = re.search(value_type_regex('def_readwrite_static'), line)
            if m:
                assert (tree['__type'] == 'class')
                tree['__static'].append((m[2], m[3]))
                return
            m = re.search(value_type_regex('def_readonly'), line)
            if m:
                assert (tree['__type'] == 'class')
                tree['__readonly'].append((m[2], m[3]))
                return
            m = re.search(r'^\s*(' + mod + r')?\.def\(\s*py::init\s*<\s*(.*)>\(', line)
            if m:
                assert (tree['__type'] == 'class')
                tree['__methods'].append(("__construct__", m[2]))
                return
            m = re.search(r'^\s*(' + mod + r')?\.def\(\s*"(\w+)"\s*\,', line)
            if m:
                assert (tree['__type'] == 'class')
                tree['__methods'].append((m[2], None))
                return

        if mod:
            parse_values()

        open_block = line.count('{')
        close_block = line.count('}')
        in_block += open_block - close_block

        if parent and re.search(r';$', line):
            if in_block == tree['__block']:
                tree = parent
    return root


def find_cpp(name, cpp_items):
    output = []
    for c in cpp_items:
        if name in c[0]:
            output.append(c)
    return output


def find(name, tree, cpp_functions):
    output = []

    if isinstance(tree, list):
        it = tree.__iter__()
    elif isinstance(tree, dict):
        it = tree.values().__iter__()

    for v in it:
        if isinstance(v, tuple):
            if name in v[0] or v[1] is not None and name in v[1]:
                cpp = v[1].split('::')[-1] if v[1] is not None else (''.join(
                    [x.capitalize() for x in v[0].split('_')])).lower()
                full_names = find_cpp(cpp, cpp_functions)
                if len(full_names) > 0:
                    for f in full_names:
                        output.append((v[0], f))
                else:
                    output.append((v[0], (cpp, "??")))
        elif isinstance(v, str):
            if name in v:
                full_names = find_cpp(v, cpp_functions)
                if len(full_names) > 0:
                    for f in full_names:
                        output.append((v, f))
                else:
                    output.append((v, ("??", "??")))
        elif hasattr(v, '__iter__'):
            r = find(name, v, cpp_functions)
            if r:
                output.extend(r)
    return output


def find_tagged(cpp_data):
    output = []
    found = None
    for line in cpp_data:
        m = re.match(r'\s*///\s*(.*)', line)
        if m:
            found = m[1]
        elif found:
            tagged = re.sub(r'^\s*[A-Z_]*\s*', '', (line.rstrip(';\n')))
            output.append((tagged, found))
            found = None
    return output


def decorate(value):
    header = html.escape(' '.join([x.capitalize() for x in value[0].split('_')]))
    cpp = html.escape(value[1][0])
    description = html.escape(value[1][1])
    py = html.escape(value[0])
    out = '<h4 class="header">' + header + '</h4>\n<p>'
    out += '<span class="tag">C++: </span><span class="source">' + cpp + '</span><br/>'
    out += '<span class="tag">Python: </span><span class="source">' + py + '</span>'
    out += '</p><p class="desc">' + description + '</p>\n'
    return out


def main(py_cpp_file, cpp_headers):
    with open(py_cpp_file) as p:
        py_tree = parse_py11_cpp(p)

    cpp_functions = []
    for cpp_header in cpp_headers:
        with open(cpp_header) as c:
            cpp_functions.extend(find_tagged(c))

    ui_file = 'make_header.html'
    file_map, names = resource.from_file(ui_file)
    ui = Gempyre.Ui(file_map, '/make_header.html', Gempyre.os_browser())
    input_element = Gempyre.Element(ui, 'input')

    def get_input(e):
        input_text = e.properties['value']
        declarations = find(input_text, py_tree, cpp_functions)
        output_element = Gempyre.Element(ui, 'output')
        if len(declarations) > 0:
            html = ""
            for d in declarations:
                html += decorate(d)
            output_element.set_html(html)
        else:
            output_element.set_html("Not found '" + input_text + "'")

    input_element.subscribe('change', get_input, ['value'])
    ui.run()


if __name__ == "__main__":
    cpp_files = []
    for o in sys.argv[2:]:
        cpp_files.extend(glob.glob(o))
    main(sys.argv[1], cpp_files)

