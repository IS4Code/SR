function shouldHandleKey(e)
{
  if (Game_StartOverlayActive)
  {
    if (e.type === 'keydown' && (e.key === 'Enter' || e.key === ' ')) return false;
    return true;
  }
  if (Game_VideoOverlayActive)
  {
    if (e.type === 'keydown' && e.key === 'Escape' && Game_VideoOverlayFinish) Game_VideoOverlayFinish();
    return true;
  }
  if (!document.body.classList.contains('page-game')) return true;
  if (e.shiftKey || e.altKey || e.metaKey) return false;
  if (e.key === 'F11') return !e.ctrlKey;
  if (e.key === 'F5') return true;
  return false;
}

function handleKeyEvent(e)
{
  if (shouldHandleKey(e))
  {
    e.stopPropagation();
  }
}
window.addEventListener('keydown', handleKeyEvent, true);
window.addEventListener('keyup', handleKeyEvent, true);
window.addEventListener('keypress', handleKeyEvent, true);

function handleBeforeUnload(e)
{
  e.preventDefault();
  e.returnValue = "Remember to save the game before exiting the page.";
}
window.addEventListener('beforeunload', handleBeforeUnload);

var Game_Pages = ['game', 'settings', 'log'];
var Game_PageButtons = {
  game: document.getElementById('btn-game'),
  settings: document.getElementById('btn-settings'),
  log: document.getElementById('btn-log'),
};

function showPanel(page)
{
  if (page === true) page = 'log';
  else if (page === false) page = 'game';

  Game_Pages.forEach(function (p) {
    document.body.classList.toggle('page-' + p, p === page);
    if (Game_PageButtons[p]) Game_PageButtons[p].classList.toggle('active', p === page);
  });
}

Game_Pages.forEach(function (p) {
  if (Game_PageButtons[p]) Game_PageButtons[p].addEventListener('click', function () { showPanel(p); });
});

(function () {
  var topbar = document.getElementById('topbar');

  function isOverTopbar(x, y) {
    var r = topbar.getBoundingClientRect();
    return x >= r.left && x < r.right && y >= r.top && y < r.bottom;
  }

  window.addEventListener('mousemove', function (e) {
    document.body.classList.toggle('ui-idle', !isOverTopbar(e.clientX, e.clientY));
  }, true);
})();
