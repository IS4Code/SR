// skip on workers
if (!Module.ENVIRONMENT_IS_PTHREAD)
{
    Module.preRun = Module.preRun || [];
    Module.preRun.push(function () {
        var dependency = 'game-manifest';
        addRunDependency(dependency);
    
        fetch("data/manifest.json").then(function (response) {
            if (!response.ok)
            {
                throw new Error(response.status);
            }
            return response.json();
        }).then(function (manifest) {
            Module.GameManifest = manifest;
    
            for (var path in manifest)
            {
                if (!manifest.hasOwnProperty(path)) continue;
    
                var pos = path.lastIndexOf("/");
                var dirPath = (pos !== -1) ? path.substring(0, pos) : "";
    
                try
                {
                    if (dirPath) FS.mkdirTree("/" + dirPath);
                    // zero-fill to report correct size before opening
                    FS.writeFile("/" + path, new Uint8Array(manifest[path]));
                }
                catch (e)
                {
                    console.error(path + " placeholder could not be created: " + e);
                    continue;
                }
            }
        }).catch(function (e) {
            console.error("manifest could not be loaded: " + e);
        }).finally(function () {
            removeRunDependency(dependency);
        });
    });
}
