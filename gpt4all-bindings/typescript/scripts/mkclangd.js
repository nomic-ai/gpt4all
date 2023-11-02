/// makes compile_flags.txt for clangd server support with this project
/// run this with typescript as your cwd

const nodeaddonapi=require('node-addon-api').include;

const findnodeapih = () => {
    const platform = process.platform;
    switch(platform) {
        case 'win32': {
            
        };
        case 'linux': {

        };
        case 'darwin': {

        };
    }
};

const knownIncludes = [
    '-I',
    './',
    '-I',
    nodeaddonapi.substring(1, nodeaddonapi.length-1),
    '-I',
    '../../gpt4all-backend'
];
const knownFlags = [
    "-x",
    "c++",
    '-std=c++17'
];

const fsp = require('fs/promises');

const output = knownFlags.join('\n')+'\n'+knownIncludes.join('\n');

fsp.writeFile('./compile_flags.txt', output, 'utf8')
    .then(() => console.log('done'))
    .catch(() => console.err('failed'));

