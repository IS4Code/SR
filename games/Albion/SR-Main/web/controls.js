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
  if (document.body.classList.contains('show-log')) return true;
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

window.addEventListener('beforeunload', function (e) {
  e.preventDefault();
  e.returnValue = "Remember to save the game before exiting the page.";
});

var btnGame = document.getElementById('btn-game');
var btnLog = document.getElementById('btn-log');

function showPanel(log)
{
  document.body.classList.toggle('show-log', log);
  btnGame.classList.toggle('active', !log);
  btnLog.classList.toggle('active', log);
}
btnGame.addEventListener('click', function () { showPanel(false); });
btnLog.addEventListener('click', function () { showPanel(true); });
