/// makes compile_flags.txt for clangd server support with this project
/// run this with typescript as your cwd
//
//for debian users make sure to install libstdc++-12-dev

const nodeaddonapi=require('node-addon-api').include;

const fsp = require('fs/promises');
const { existsSync, readFileSync } = require('fs');
const assert = require('node:assert');
const findnodeapih = () => {
    assert(existsSync("./build"), "Haven't built the application once yet. run node scripts/prebuild.js");
    const dir = readFileSync("./build/config.gypi", 'utf8');
    const nodedir_line = dir.match(/"nodedir": "([^"]+)"/);
    assert(nodedir_line, "Found no matches")
    assert(nodedir_line[1]);
    console.log("node_api.h found at: ", nodedir_line[1]);
    return nodedir_line[1]+"/include/node";
};

const knownIncludes = [
    '-I',
    './',
    '-I',
    nodeaddonapi.substring(1, nodeaddonapi.length-1),
    '-I',
    '../../gpt4all-backend',
    '-I',
    findnodeapih()
];
const knownFlags = [
    "-x",
    "c++",
    '-std=c++17'
];


const output = knownFlags.join('\n')+'\n'+knownIncludes.join('\n');

fsp.writeFile('./compile_flags.txt', output, 'utf8')
    .then(() => console.log('done'))
    .catch(() => console.err('failed'));

