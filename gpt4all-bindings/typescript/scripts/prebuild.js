const prebuildify = require("prebuildify");

async function createPrebuilds(configs) {
    for (const config of configs) {
        const opts = {
            napi: true,
            targets: ["18.16.0"],
            ...config,
        };
        try {
            await createPrebuild(opts);
            console.log(
                `Build succeeded for platform ${opts.platform} and architecture ${opts.arch}`,
            );
        } catch (err) {
            console.error(
                `Error building for platform ${opts.platform} and architecture ${opts.arch}:`,
                err,
            );
        }
    }
}

function createPrebuild(opts) {
    return new Promise((resolve, reject) => {
        // if this prebuild is cross-compiling for arm64 on a non-arm64 machine,
        // set the CXX and CC environment variables to the cross-compilers
        if (
            opts.arch === "arm64" &&
            process.arch !== "arm64" &&
            process.platform === "linux"
        ) {
            process.env.CXX = "aarch64-linux-gnu-g++-12";
            process.env.CC = "aarch64-linux-gnu-gcc-12";
        }

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
if (process.platform === "win32") {
    prebuildConfigs = [{ platform: "win32", arch: "x64" }];
} else if (process.platform === "linux") {
    prebuildConfigs = [
        { platform: "linux", arch: "x64" },
        { platform: "linux", arch: "arm64" },
    ];
} else if (process.platform === "darwin") {
    prebuildConfigs = [
        { platform: "darwin", arch: "x64" },
        { platform: "darwin", arch: "arm64" },
    ];
}

createPrebuilds(prebuildConfigs)
    .then(() => console.log("All builds succeeded"))
    .catch((err) => console.error("Error building:", err));
