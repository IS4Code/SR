var statusElement = document.getElementById('status-text');

var Module = {
  preRun: [],
  postRun: [],
  print: (function () {
    var element = document.getElementById('output');
    if (element) element.value = '';
    return function (text) {
      if (arguments.length > 1) text = Array.prototype.slice.call(arguments).join(' ');
      console.log(text);
      if (element)
      {
        element.value += text + "\n";
        element.scrollTop = element.scrollHeight;
      }
    };
  })(),
  canvas: (function () {
    var canvas = document.getElementById('canvas');
    canvas.addEventListener('webglcontextlost', function (e) { alert("WebGL context lost. Please reload the page."); e.preventDefault(); }, false);
    return canvas;
  })(),
  setStatus: function (text) {
    if (!Module.setStatus.last) Module.setStatus.last = { time: Date.now(), text: '' };
    if (text === Module.setStatus.last.text) return;
    var m = text.match(/([^(]+)\((\d+(\.\d+)?)\/(\d+)\)/);
    var now = Date.now();
    if (m && now - Module.setStatus.last.time < 30) return; // skip early progress update
    Module.setStatus.last.time = now;
    Module.setStatus.last.text = text;
    if (m) {
      var cur = parseFloat(m[2]), total = parseFloat(m[4]);
      if (total >= 1000000)
      {
        text = m[1] + "(" + (cur / 1048576).toFixed(1) + "/" + (total / 1048576).toFixed(1) + " MB)";
      }
      else
      {
        text = m[1] + "(" + m[2] + '/' + m[4] + ")";
      }
    }
    statusElement.innerHTML = text;
  },
  totalDependencies: 0,
  monitorRunDependencies: function (left) {
    this.totalDependencies = Math.max(this.totalDependencies, left);
    Module.setStatus(left ? "Preparing... (" + (this.totalDependencies - left) + "/" + this.totalDependencies + ")" : "");
  }
};
Module.setStatus("Downloading...");
window.onerror = function (message, source, lineno, colno, error) {
  var text = "Uncaught exception: " + message;
  if (source) text += "\n    at " + source + ":" + lineno + (colno ? ":" + colno : "");
  if (error && error.stack) text += "\n" + error.stack;

  Module.print(text);
  showPanel(true);

  statusElement.innerHTML = "JS Error.";
  Module.setStatus = function () { };
};
