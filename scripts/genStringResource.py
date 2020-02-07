import sys
import base64
import re
import os
import argparse


app = os.path.abspath(sys.argv[0])
appdir = os.path.dirname(app)

sys.path.append(appdir + '/jsmin-2.2.2')
from jsmin import jsmin

parser = argparse.ArgumentParser()
parser.add_argument('target')
parser.add_argument('sources', nargs='+')
parser.add_argument('-m', '--minify', dest='minify')
args = parser.parse_args()

print("GSR", args.target, args.sources)

outName = args.target
directory = os.path.dirname(outName)
if not os.path.exists(directory):
    os.makedirs(directory)
        
with open(outName, 'w') as file:
    cname = re.sub('[^a-zA-Z_]', '_', os.path.basename(outName)).upper()
    file.write('#ifndef ' + cname + '\n')
    file.write('#define ' + cname + '\n')
    file.write('#include <string>\n')
    
    for inName in args.sources:  
        domin = True if args.minify and os.path.splitext(inName) == 'js' else False

        encoded = None

        with open(inName, 'rb') as infile:
            content = jsmin(infile.read()) if domin else infile.read()
            encoded = base64.standard_b64encode(content).decode('utf-8')

    
        sname = re.sub('[^a-zA-Z_]', '', os.path.basename(inName)).capitalize()

        chunksize = 1024 
        data = [encoded [ i - chunksize : i ] for i in range(chunksize, len(encoded) + chunksize , chunksize)]  

        file.write('const std::string ' + sname + '=\nstd::string("')
        file.write('") +\nstd::string("'.join(data))
        file.write('");\n')
        
    file.write('#endif //' + cname +  '\n')




