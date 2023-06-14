//Maybe some command line piping would work better, but can't think of platform independent command line tool
const fs = require('fs');
const filepath = 'README.md';
const data = fs.readFileSync(filepath);
fs.writeFileSync("docs/api.md", data);
