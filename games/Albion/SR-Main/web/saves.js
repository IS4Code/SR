var saveNamePattern = /^SAVE\.\d{3}$/i;

var crc32Table = (function () {
  var table = new Uint32Array(256);
  for (var n = 0; n < 256; n++) {
    var c = n;
    for (var k = 0; k < 8; k++) {
      c = (c & 1) ? (0xedb88320 ^ (c >>> 1)) : (c >>> 1);
    }
    table[n] = c >>> 0;
  }
  return table;
})();

function crc32(bytes)
{
  var crc = 0xffffffff;
  for (var i = 0; i < bytes.length; i++) {
    crc = crc32Table[(crc ^ bytes[i]) & 0xff] ^ (crc >>> 8);
  }
  return (crc ^ 0xffffffff) >>> 0;
}

function zu16(v) { return new Uint8Array([v & 0xff, (v >> 8) & 0xff]); }
function zu32(v) { return new Uint8Array([v & 0xff, (v >> 8) & 0xff, (v >> 16) & 0xff, (v >>> 24) & 0xff]); }

function dosDateTime()
{
  var d = new Date();
  return {
    time: ((d.getHours() & 0x1f) << 11) | ((d.getMinutes() & 0x3f) << 5) | ((d.getSeconds() >> 1) & 0x1f),
    date: (((d.getFullYear() - 1980) & 0x7f) << 9) | (((d.getMonth() + 1) & 0xf) << 5) | (d.getDate() & 0x1f)
  };
}

async function buildZip(files)
{
  var dt = dosDateTime();
  var localParts = [];
  var centralParts = [];
  var offset = 0;

  for (var f = 0; f < files.length; f++)
  {
    var file = files[f];
    var nameBytes = new TextEncoder().encode(file.name);
    var crc = crc32(file.data);

    var method = 0;
    var stored = file.data;
    if (compressionSupported)
    {
      try
      {
        stored = await deflateRaw(stored);
        method = 8;
      }
      catch (e)
      {
        Module.print("Could not compress " + file.name + " - storing uncompressed. (" + e + ")");
      }
    }

    var localOffset = offset;
    var localHeader = [
      zu32(0x04034b50), zu16(20), zu16(0), zu16(method), zu16(dt.time), zu16(dt.date),
      zu32(crc), zu32(stored.length), zu32(file.data.length), zu16(nameBytes.length), zu16(0),
      nameBytes, stored
    ];
    localHeader.forEach(function (p) { localParts.push(p); offset += p.length; });

    centralParts.push([
      zu32(0x02014b50), zu16(20), zu16(20), zu16(0), zu16(method), zu16(dt.time), zu16(dt.date),
      zu32(crc), zu32(stored.length), zu32(file.data.length), zu16(nameBytes.length), zu16(0), zu16(0),
      zu16(0), zu16(0), zu32(0), zu32(localOffset), nameBytes
    ]);
  }

  var centralOffset = offset;
  var flatCentral = [];
  centralParts.forEach(function (entry) {
    entry.forEach(function (p) { flatCentral.push(p); offset += p.length; });
  });
  var centralSize = offset - centralOffset;

  var eocd = [
    zu32(0x06054b50), zu16(0), zu16(0), zu16(files.length), zu16(files.length),
    zu32(centralSize), zu32(centralOffset), zu16(0)
  ];

  return new Blob(localParts.concat(flatCentral).concat(eocd), { type: 'application/zip' });
}

var compressionSupported = typeof CompressionStream !== 'undefined';

async function deflateRaw(bytes)
{
  var stream = new Blob([bytes]).stream().pipeThrough(new CompressionStream('deflate-raw'));
  return new Uint8Array(await new Response(stream).arrayBuffer());
}

async function inflateRaw(bytes)
{
  var stream = new Blob([bytes]).stream().pipeThrough(new DecompressionStream('deflate-raw'));
  return new Uint8Array(await new Response(stream).arrayBuffer());
}

async function parseZip(arrayBuffer)
{
  var bytes = new Uint8Array(arrayBuffer);
  var view = new DataView(arrayBuffer);
  var u16 = function (o) { return view.getUint16(o, true); };
  var u32 = function (o) { return view.getUint32(o, true); };

  // find eocd
  var eocdOffset = -1;
  var searchStart = Math.max(0, bytes.length - 22 - 65535);
  for (var i = bytes.length - 22; i >= searchStart; i--)
  {
    if (u32(i) === 0x06054b50) { eocdOffset = i; break; }
  }
  if (eocdOffset < 0) throw new Error("invalid ZIP file (EOCD record not found)");

  var entryCount = u16(eocdOffset + 10);
  var pos = u32(eocdOffset + 16); // central directory offset
  var entries = [];

  for (var e = 0; e < entryCount; e++)
  {
    if (u32(pos) !== 0x02014b50) throw new Error("invalid ZIP file (corrupted ZIP central directory)");
    var method = u16(pos + 10);
    var compSize = u32(pos + 20);
    var nameLen = u16(pos + 28);
    var extraLen = u16(pos + 30);
    var commentLen = u16(pos + 32);
    var localOffset = u32(pos + 42);
    var name = new TextDecoder().decode(bytes.subarray(pos + 46, pos + 46 + nameLen));

    entries.push({ name: name, method: method, compSize: compSize, localOffset: localOffset });
    pos += 46 + nameLen + extraLen + commentLen;
  }

  var result = [];
  for (var j = 0; j < entries.length; j++)
  {
    var ent = entries[j];
    if (ent.name.charAt(ent.name.length - 1) === '/') continue; // directory entry

    var lp = ent.localOffset;
    if (u32(lp) !== 0x04034b50) throw new Error("invalid ZIP file (" + ent.name + " has corrupted local header)");
    var dataStart = lp + 30 + u16(lp + 26) + u16(lp + 28);
    var compData = bytes.subarray(dataStart, dataStart + ent.compSize);

    var raw;
    if (ent.method === 0)
    {
      raw = compData;
    }
    else if (ent.method === 8)
    {
      raw = await inflateRaw(compData);
    }
    else
    {
      Module.print("Skipping " + ent.name + " for import - unsupported ZIP compression method " + ent.method);
      continue;
    }

    result.push({ name: ent.name, data: raw });
  }

  return result;
}

function eraseDirRecursive(FS, path)
{
  var entries;
  try { entries = FS.readdir(path); } catch (e) { return; }
  entries.forEach(function (name) {
    if (name === '.' || name === '..') return;
    var full = path + '/' + name;
    var st;
    try { st = FS.stat(full); } catch (e) { return; }
    if (FS.isDir(st.mode)) {
      eraseDirRecursive(FS, full);
      try { FS.rmdir(full); } catch (e) {}
    } else {
      try { FS.unlink(full); } catch (e) {}
    }
  });
}

async function exportSaves()
{
  if (!Module.FS)
  {
    Module.print("Filesystem not ready yet for save game export.");
    showPanel(true);
    return;
  }
  var FS = Module.FS;
  var names;
  try
  {
    names = FS.readdir("/SAVES");
  }
  catch (e)
  {
    Module.print("/SAVES could not be opened (" + e + ").");
    showPanel(true);
    return;
  }

  var files = [];
  names.forEach(function (name) {
    if (!saveNamePattern.test(name)) return;
    var full = '/SAVES/' + name;
    var st;
    try { st = FS.stat(full); } catch (e) { return; }
    if (!FS.isFile(st.mode)) return;
    files.push({ name: name, data: FS.readFile(full) });
  });

  if (files.length === 0)
  {
    Module.print('No save files found for export.');
    showPanel(true);
    return;
  }

  var blob = await buildZip(files);
  var url = URL.createObjectURL(blob);
  var a = document.createElement('a');
  a.href = url;
  a.download = "saves.zip";
  document.body.appendChild(a);
  a.click();
  document.body.removeChild(a);
  setTimeout(function () { URL.revokeObjectURL(url); }, 10000);

  Module.print("Exported " + files.length + " save files.");
}

async function importSavesFile(file)
{
  if (!Module.FS)
  {
    Module.print("Filesystem not ready yet for save game import.");
    showPanel(true);
    return;
  }
  var FS = Module.FS;

  var entries;
  try
  {
    entries = await parseZip(await file.arrayBuffer());
  }
  catch (e)
  {
    Module.print("Could not read save game import ZIP: " + e);
    showPanel(true);
    return;
  }

  var saves = entries.filter(function (ent) {
    return ent.name.indexOf('/') === -1 && saveNamePattern.test(ent.name);
  });

  if (saves.length === 0)
  {
    Module.print("No save games found in " + file.name + ".");
    showPanel(true);
    return;
  }

  if (!window.confirm("Importing " + saves.length + " saves will erase all previous saved positions. Continue?"))
  {
    return;
  }

  eraseDirRecursive(FS, "/SAVES");
  saves.forEach(function (ent) { FS.writeFile("/SAVES/" + ent.name, ent.data); });

  FS.syncfs(false, function (err) {
    if (err)
    {
      Module.print("Imported saves failed to sync: " + err);
      showPanel(true);
    }
    else
    {
      Module.print("Imported " + saves.length + " saves from " + file.name + ".");
    }
  });
}

document.getElementById('btn-export-saves').addEventListener('click', exportSaves);

var importSavesInput = document.getElementById('import-saves-input');
document.getElementById('btn-import-saves').addEventListener('click', function () {
  importSavesInput.value = "";
  importSavesInput.click();
});
importSavesInput.addEventListener('change', function () {
  if (importSavesInput.files.length > 0) importSavesFile(importSavesInput.files[0]);
});
