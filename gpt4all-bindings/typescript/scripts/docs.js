//Maybe some command line piping would work better, but can't think of platform independent command line tool

const fs = require('fs');
const preOrPost = process.env.npm_lifecycle_event; 

const intermediatePath = 'docs/api.md';
if(preOrPost === 'predocs:build') {
    const filepath = 'README.md';
    const data = fs.readFileSync(filepath);
    fs.writeFileSync(intermediatePath, data);
} else if(preOrPost === 'postdocs:build') {
    const newPath = '../python/docs/gpt4all_typescript.md';
    fs.rename(intermediatePath, newPath, (err) => {
        if(err) throw err;
        console.log('Successful move to python directory')
    })
}


