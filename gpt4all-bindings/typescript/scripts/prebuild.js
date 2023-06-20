const prebuildify = require("prebuildify");

async function createPrebuilds(combinations) {
    for (const { platform, arch } of combinations) {
        const opts = {
            platform,
            arch,
            napi: true,
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

const prebuildConfigs = [
    { platform: "win32", arch: "x64" },
    { platform: "win32", arch: "arm64" },
    // { platform: 'win32', arch: 'armv7' },
    { platform: "darwin", arch: "x64" },
    { platform: "darwin", arch: "arm64" },
    // { platform: 'darwin', arch: 'armv7' },
    { platform: "linux", arch: "x64" },
    { platform: "linux", arch: "arm64" },
    { platform: "linux", arch: "armv7" },
];

createPrebuilds(prebuildConfigs)
    .then(() => console.log("All builds succeeded"))
    .catch((err) => console.error("Error building:", err));
