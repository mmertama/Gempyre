import sys
import base64
import re
import os
import argparse

def main():
    app = os.path.abspath(sys.argv[0])
    appdir = os.path.dirname(app)

    sys.path.append(appdir + '/jsmin-2.2.2')
    from jsmin import jsmin

    parser = argparse.ArgumentParser()
    parser.add_argument('target')
    parser.add_argument('sources', nargs='+')
    parser.add_argument('-m', '--minify', dest='minify')
    args = parser.parse_args()

    outName = args.target
    directory =  os.path.dirname(outName) if '/' in outName else "."
    if not os.path.exists(directory):
        os.makedirs(directory)

    with open(outName, 'w') as file:
        cname = re.sub('[^a-zA-Z_]', '_', os.path.basename(outName)).upper()
        file.write('#ifndef ' + cname + '\n')
        file.write('#define ' + cname + '\n')
        file.write('#include <string>\n')
        file.write('#include <unordered_map>\n')

        item_list = {}

        for inName in args.sources:
            domin = True if args.minify and os.path.splitext(inName) == 'js' else False

            encoded = None

            with open(inName, 'rb') as infile:
                content = jsmin(infile.read()) if domin else infile.read()
                encoded = base64.standard_b64encode(content).decode('utf-8')

            sname = re.sub('[^0-9a-zA-Z_]', '', os.path.basename(inName)).capitalize()

            c_name = os.path.basename(inName)

            if c_name in item_list:
                print(f"Fatal error: duplicate name '{inName}'")
                exit(-1)

            item_list[c_name] = sname

            chunksize = 1024
            data = [encoded[i - chunksize: i] for i in range(chunksize, len(encoded) + chunksize, chunksize)]

            file.write('const std::string ' + sname + ' =\nstd::string("')
            file.write('") +\nstd::string("'.join(data))
            file.write('");\n')

        list_name = re.sub('[^a-zA-Z_]', '', os.path.basename(outName)).capitalize()
        file.write('const std::unordered_map<std::string, std::string> ' + list_name + '{\n')
        for k, v in item_list.items():
            file.write('\t{"/' + k + '", ' + v + '},\n')
        file.write('};\n')

        file.write('#endif //' + cname + '\n')


if __name__ == "__main__":
    main()

