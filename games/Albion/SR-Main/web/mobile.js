function Game_IsPhone() {
  try {
    if (navigator.userAgentData && typeof navigator.userAgentData.mobile === "boolean") {
      return navigator.userAgentData.mobile;
    }
  } catch (e) {}
  return /Android|webOS|iPhone|iPad|iPod|BlackBerry|Windows Phone/i.test(navigator.userAgent);
}

document.body.classList.toggle("is-phone", Game_IsPhone());

function ccallSafeArgs(name, args) {
  if (Game_RuntimeReady && Module.ccall) {
    var types = args.map(function () { return "number"; });
    try { Module.ccall(name, null, types, args); } catch (e) {}
  }
}

function ccallSafeRet(name, args) {
  if (Game_RuntimeReady && Module.ccall) {
    var types = args.map(function () { return "number"; });
    try { return Module.ccall(name, "number", types, args); } catch (e) {}
  }
  return 0;
}

var SDLK_ESCAPE = 27;
var SDLK_TAB = 9;

function preventFocusSteal(button) {
  button.addEventListener("mousedown", function (e) { e.preventDefault(); });
}

var btnMobileMenu = document.getElementById("btn-mobile-menu");
if (btnMobileMenu) {
  preventFocusSteal(btnMobileMenu);
  btnMobileMenu.addEventListener("click", function () {
    ccallSafeArgs("Game_InjectKey", [SDLK_ESCAPE]);
  });
}

var btnMobileMap = document.getElementById("btn-mobile-map");
if (btnMobileMap) {
  preventFocusSteal(btnMobileMap);
  btnMobileMap.addEventListener("click", function () {
    ccallSafeArgs("Game_InjectKey", [SDLK_TAB]);
  });
}

var btnMobileSelection = document.getElementById("btn-mobile-selection");
if (btnMobileSelection) {
  preventFocusSteal(btnMobileSelection);
  btnMobileSelection.addEventListener("click", function () {
    var on = ccallSafeRet("Game_SelectionMode_Toggle", []);
    btnMobileSelection.classList.toggle("active", !!on);
  });
}

var btnFullscreen = document.getElementById("btn-fullscreen");
if (btnFullscreen) {
  btnFullscreen.addEventListener("click", function () {
    if (document.fullscreenElement) {
      if (document.exitFullscreen) document.exitFullscreen().catch(function () {});
    } else if (document.documentElement.requestFullscreen) {
      document.documentElement.requestFullscreen().catch(function () {});
    }
  });
  document.addEventListener("fullscreenchange", function () {
    btnFullscreen.classList.toggle("active", !!document.fullscreenElement);
  });
}

var mobileKbdTrigger = document.getElementById("mobile-kbd-trigger");
if (mobileKbdTrigger) {
  mobileKbdTrigger.addEventListener("input", function (e) {
    var type = e.inputType || "";

    if (type === "deleteContentBackward" || type === "deleteContentForward") {
      ccallSafeArgs("Game_InjectChar", [8]);
    } else if (type === "insertLineBreak") {
      ccallSafeArgs("Game_InjectChar", [13]);
    } else {
      var value = mobileKbdTrigger.value;
      for (var i = 0; i < value.length; i++) {
        var code = value.charCodeAt(i);
        if (code < 128) ccallSafeArgs("Game_InjectChar", [code]);
      }
    }

    mobileKbdTrigger.value = "";
  });
}
