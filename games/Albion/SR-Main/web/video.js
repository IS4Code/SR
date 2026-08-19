var videoOverlay = document.getElementById('video-overlay');
Game_VideoOverlayActive = false;
Game_VideoOverlayFinish = null;
Game_RuntimeReady = false;

function ccallSafe(name) {
  if (Game_RuntimeReady && Module.ccall) {
    try { Module.ccall(name, null, [], []); } catch (e) {}
  }
}

var Game_StartRequested = false;
var Game_MainCalled = false;
function Game_MaybeStartGame() {
  if (!Game_StartRequested || !Game_RuntimeReady || Game_MainCalled) return;
  Game_MainCalled = true;
  try {
    if (typeof Game_ApplyBootOverrides === 'function') Game_ApplyBootOverrides();
  } catch (e) {
    console.error("Config override failed: " + e);
  }
  if (Module.callMain) Module.callMain();
}

function Game_ShowVideoOverlay(url, onFinish, fadeConfig, volume) {
  var finished = false;
  var stopped = false;
  var lastProgress = Date.now();
  var fadeTimer = null;

  function stop() {
    if (stopped) return;
    stopped = true;
    videoOverlay.pause();
    document.body.classList.remove('show-video', 'fading');
    videoOverlay.style.transition = '';
    videoOverlay.style.opacity = '';
    videoOverlay.removeEventListener('ended', onEnded);
    videoOverlay.removeEventListener('error', onError);
    videoOverlay.removeEventListener('timeupdate', onTimeUpdate);
    clearInterval(freezeTimer);
    if (fadeTimer) clearTimeout(fadeTimer);
  }

  function finish() {
    if (finished) return;
    finished = true;
    Game_VideoOverlayActive = false;
    Game_VideoOverlayFinish = null;
    document.body.classList.remove('video-blocking');
    if (onFinish) onFinish();
  }

  function done(reason) {
    finish();
    stop();
  }

  function onEnded() { done('ended'); }
  function onError() { done('error'); }
  function onTimeUpdate() {
    lastProgress = Date.now();
    if (fadeConfig && !finished && videoOverlay.currentTime >= fadeConfig.at) {
      videoOverlay.style.transition = 'opacity ' + fadeConfig.duration + 's linear';
      document.body.classList.add('fading');
      videoOverlay.style.opacity = '0';
      ccallSafe('Game_WebVideo_Resume');
      fadeTimer = setTimeout(finish, fadeConfig.duration * 1000);
    }
  }

  videoOverlay.addEventListener('ended', onEnded);
  videoOverlay.addEventListener('error', onError);
  videoOverlay.addEventListener('timeupdate', onTimeUpdate);

  var freezeTimer = setInterval(function () {
    if (Date.now() - lastProgress > 10000) done('froze');
  }, 2000);

  Game_VideoOverlayActive = true;
  Game_VideoOverlayFinish = function () { done('esc'); };
  if (typeof showPanel === 'function') showPanel('game');
  document.body.classList.add('show-video', 'video-blocking');

  videoOverlay.currentTime = 0;
  videoOverlay.volume = 0.5 * ((volume == null) ? 1.0 : volume);
  videoOverlay.src = url;

  videoOverlay.play().catch(function () { done('failed'); });
}

function Game_IntroVolume()
{
  var raw = (typeof Game_GetCookie === 'function') ? Game_GetCookie('alb_opt_MIDI_VOLUME') : null;
  var n = parseInt(raw, 10);
  if (!(n >= 0)) n = 64;
  return Math.max(0, Math.min(1, n / 128));
}

function Game_PrepareIntro()
{
  Game_StartOverlayActive = true;

  document.body.classList.add('video-blocking');
  var startOverlay = document.getElementById('start-overlay');
  var btnStart = document.getElementById('btn-start');
  btnStart.focus();
  btnStart.addEventListener('click', function () {
    Game_StartOverlayActive = false;
    startOverlay.classList.add('hidden');
    ccallSafe('Game_WebVideo_Pause');
    Game_ShowVideoOverlay(
      'data/VIDEO/INTRO.MP4',
      function () { ccallSafe('Game_WebVideo_Resume'); },
      { at: 156.250, duration: 1.175 },
      Game_IntroVolume()
    );
    Game_StartRequested = true;
    setTimeout(Game_MaybeStartGame, 0);
  }, { once: true });
}
Game_PrepareIntro();
