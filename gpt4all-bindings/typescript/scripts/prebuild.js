const prebuildify = require("prebuildify");

async function createPrebuilds(combinations) {
    for (const { platform, arch } of combinations) {
        const opts = {
            platform,
            arch,
            napi: true,
            targets: ["18.16.0"]
        };
        try {
            await createPrebuild(opts);
            console.log(
                `Build succeeded for platform ${opts.platform} and architecture ${opts.arch}`
            );
        } catch (err) {
            console.error(
                `Error building for platform ${opts.platform} and architecture ${opts.arch}:`,
                err
            );
        }
    }
}

function createPrebuild(opts) {
    return new Promise((resolve, reject) => {
        prebuildify(opts, (err) => {
            if (err) {
                reject(err);
            } else {
                resolve();
            }
        });
    });
}

let prebuildConfigs;
if(process.platform === 'win32') {
   prebuildConfigs = [
    { platform: "win32", arch: "x64" }
   ];
} else if(process.platform === 'linux') {
   //Unsure if darwin works, need mac tester!
   prebuildConfigs = [
    { platform: "linux", arch: "x64" },
    //{ platform: "linux", arch: "arm64" },
    //{ platform: "linux", arch: "armv7" },
   ]
} else if(process.platform === 'darwin') {
    prebuildConfigs = [
       { platform: "darwin", arch: "x64" },
       { platform: "darwin", arch: "arm64" },
    ]
}

createPrebuilds(prebuildConfigs)
    .then(() => console.log("All builds succeeded"))
    .catch((err) => console.error("Error building:", err));
