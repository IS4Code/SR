// skip on workers
if (!Module.ENVIRONMENT_IS_PTHREAD)
{
    Module.preRun = Module.preRun || [];

    Module.preRun.push(function () {
        var queue = [];
        var running = false;

        function onResume(err)
        {
            if (err)
            {
                console.error(err);
            }
            if (queue.length === 0)
            {
                running = false;
                return;
            }
            (queue.shift())(onResume);
        }

        Module.SyncQueue = function (op) {
            if (running)
            {
                queue.push(op);
                return;
            }
            running = true;
            op(onResume);
        };
    });

    Module.preRun.push(function () {
        FS.mkdirTree("/XLDLIBS/CURRENT");

        // Persist /SAVES
        FS.mkdirTree("/SAVES");
        FS.mount(IDBFS, {}, "/SAVES");

        var dependency = 'fs-sync';
        addRunDependency(dependency);
        FS.syncfs(true, err => removeRunDependency(dependency));
    });

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

    Module.onRuntimeInitialized = function () {
        Game_RuntimeReady = true;

        try
        {
            var hash = location.hash;
            if (hash.length <= 1) return;

            var query = (hash.charAt(1) === '?') ? hash.substring(2) : hash.substring(1);
            var params = new URLSearchParams(query);
            var overrides = "";

            params.forEach(function (value, key) {
                overrides += key + "=" + value + "\n";
            });

            if (!overrides) return;

            var existing = "";
            try { existing = FS.readFile("/Albion.cfg", { encoding: "utf8" }); } catch (e) {}
            FS.writeFile("/Albion.cfg", existing + "\n" + overrides);
        }
        catch (e)
        {
            console.error("Config override failed: " + e);
        }

        try
        {
            if (Game_VideoOverlayActive && Module.ccall)
            {
                Module.ccall("Game_WebVideo_Pause", null, [], []);
            }
        }
        catch (e)
        {
            console.error("Video overlay pause sync failed: " + e);
        }
    };
}
