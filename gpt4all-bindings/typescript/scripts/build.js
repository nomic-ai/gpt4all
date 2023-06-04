const { spawn } = require('node:child_process');

const platform = process.platform;

//windows 64bit or 32
if(platform === 'win32') {
    const path = 'scripts/build_mingw.ps1'; 
    spawn(`pwsh`, [path], { shell: true, stdio: "inherit"});
    process.on('data',s => console.log(s.toString())); 
} else if(platform ==='linux' || platform === 'darwin') {
    const path = 'scripts/build_unix.sh'; 
    const bash = spawn(`sh`, [path]);
    bash.stdout.on('data', s => console.log(s.toString()), {stdio: "inherit"});
}
