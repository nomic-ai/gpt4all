const { spawn } = require("node:child_process");
const { resolve } = require("path");
const args = process.argv.slice(2);
const platform = process.platform;
//windows 64bit or 32
if (platform === "win32") {
    const path = "scripts/build_msvc.bat";
    spawn(resolve(path), ["/Y", ...args], { shell: true, stdio: "inherit" });
    process.on("data", (s) => console.log(s.toString()));
} else if (platform === "linux" || platform === "darwin") {
    const path = "scripts/build_unix.sh";
    spawn(`sh `, [path, args], {
        shell: true,
        stdio: "inherit",
    });
    process.on("data", (s) => console.log(s.toString()));
}
