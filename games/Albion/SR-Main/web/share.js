var btnShare = document.getElementById('btn-share');

var Game_Share_TempSavePath = '/SAVES/save.101';

function Game_Share_BuildUrl(params)
{
  return location.origin + location.pathname + location.search + '#?' + params.toString();
}

function Game_Share_ToBase64Url(bytes)
{
  if (bytes.toBase64)
  {
    return bytes.toBase64({ alphabet: 'base64url', omitPadding: true });
  }

  var chunks = [];
  var chunkSize = 0x8000;
  for (var i = 0; i < bytes.length; i += chunkSize)
  {
    chunks.push(String.fromCharCode.apply(null, bytes.subarray(i, i + chunkSize)));
  }

  var subst = { '+': '-', '/': '_', '=': '' };
  return btoa(chunks.join("")).replace(/[+/=]/g, c => subst[c]);
}

async function Game_Share_CompressFileChunked(offset, compressionStream)
{
  var chunkSize = 0x10000;
  var totalLen = Module.FS.stat(Game_Share_TempSavePath).size;
  var stream = Module.FS.open(Game_Share_TempSavePath, 'r');

  var writer = compressionStream.writable.getWriter();

  var pump = (async function ()
  {
    var buf = new Uint8Array(chunkSize);
    var pos = offset;
    while (pos < totalLen)
    {
      var want = Math.min(chunkSize, totalLen - pos);
      var got = Module.FS.read(stream, buf, 0, want, pos);
      if (got <= 0) break;
      await writer.write(buf.slice(0, got));
      pos += got;
    }
    await writer.close();
  })();

  var results = await Promise.all([ pump, new Response(compressionStream.readable).arrayBuffer() ]);
  Module.FS.close(stream);
  return new Uint8Array(results[1]);
}

async function Game_Share_BuildAndCopyLinkAsync(name, version, dataOffset, bytes, alreadyCompressed, compressionStream)
{
  try
  {
    var compressed = bytes;
    if (!alreadyCompressed)
    {
      compressed = await Game_Share_CompressFileChunked(dataOffset, compressionStream);
    }

    var dataBase64 = Game_Share_ToBase64Url(compressed);

    var params = (typeof Game_ParseFragmentParams === 'function') ? Game_ParseFragmentParams() : new URLSearchParams();
    if (name)
    {
      params.set('Save_Name', name);
    }
    else
    {
      params.delete('Save_Name');
    }
    params.set('Save_Ver', String(version));
    params.set('Save_Data', dataBase64);

    var url = Game_Share_BuildUrl(params);

    if (navigator.clipboard && navigator.clipboard.writeText)
    {
      await navigator.clipboard.writeText(url);
      if (Module.print) Module.print("Share link copied to clipboard.");
    }
    else
    {
      window.prompt("Copy this link:", url);
    }

    try { Module.FS.unlink(Game_Share_TempSavePath); } catch (e) {}
  }
  catch (e)
  {
    if (Module.print) Module.print("Failed to build link (" + e + ").");
    showPanel(true);
  }
}

window.Game_Share_BuildAndCopyLink = function (name, version, dataOffset, bytes, alreadyCompressed) {
  if (alreadyCompressed)
  {
    Game_Share_BuildAndCopyLinkAsync(name, version, dataOffset, bytes, true, null);
    return 1;
  }
  
  var compressionStream;
  try
  {
    compressionStream = new CompressionStream('brotli');
  }
  catch (e)
  {
    return 0;
  }

  Game_Share_BuildAndCopyLinkAsync(name, version, dataOffset, bytes, false, compressionStream);
  return 1;
};

if (btnShare)
{
  btnShare.addEventListener('click', function () {
    ccallSafe('Game_Share_RequestLink');
  });
}

var btnStartForShare = document.getElementById('btn-start');
if (btnStartForShare && btnShare)
{
  btnStartForShare.addEventListener('click', function () {
    btnShare.style.display = '';
  }, { once: true });
}

function Game_Share_PatchSetup()
{
  var params = (typeof Game_ParseFragmentParams === 'function') ? Game_ParseFragmentParams() : new URLSearchParams();
  if (!params.has('Save_Data')) return;

  var setupText = null;
  try { setupText = Module.FS.readFile('/SETUP.INI', { encoding: 'utf8' }); } catch (e) {}
  if (setupText != null && typeof Game_IniSetVariable === 'function')
  {
    Module.FS.writeFile('/SETUP.INI', Game_IniSetVariable(setupText, 'ALBION', 'SAVED_GAME_NR', '101'));
  }
}

function Game_Share_FromBase64Url(str)
{
  if (Uint8Array.fromBase64)
  {
    return Uint8Array.fromBase64(str, { alphabet: 'base64url' });
  }
  var subst = { '-': '+', '_': '/' };
  subst[''] = '='.repeat((4 - str.length % 4) % 4);
  var base64 = str.replace(/[-_]|$/g, c => subst[c]);
  var binary = atob(base64);
  var bytes = new Uint8Array(binary.length);
  for (var i = 0; i < binary.length; i++) bytes[i] = binary.charCodeAt(i);
  return bytes;
}
