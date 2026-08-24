var Game_SettingsFields = [
  {
    key: 'Audio_MIDI_Subsystem', label: 'MIDI subsystem', group: 'audio', type: 'select', default: 'fluidsynth',
    options: [['fluidsynth', 'FluidSynth'], ['adlmidi', 'ADLMIDI (OPL3)']]
  },

  { key: 'Display_Enhanced_3D_Rendering', label: 'Enhanced 3D rendering', group: 'display', type: 'onoff', default: 'on' },
  {
    key: 'Display_Scaling', label: 'Scaling filter', group: 'display', type: 'select', default: 'advanced',
    options: [['advanced', 'Bilinear'], ['advancednb', 'Nearest neighbor']]
  },
  // {
  //   key: 'Display_AdvancedScaler', label: 'Pixel upscaler', group: 'display', type: 'select', default: 'normal',
  //   options: [['normal', 'Nearest-neighbour'], ['hqx', 'HQx'], ['xbrz', 'xBRZ']]
  // },
  {
    key: 'Display_ScalerFactor', label: '3D resolution factor', group: 'display', type: 'select', default: '6',
    options: [['max', 'Maximum'], ['2', '2x'], ['3', '3x'], ['4', '4x'], ['5', '5x'], ['6', '6x']]
  },
  { key: 'Display_ScaledWidth', label: 'Window width', group: 'display', type: 'number', default: '1080', min: 320 },
  { key: 'Display_ScaledHeight', label: 'Window height', group: 'display', type: 'number', default: '720', min: 240 },
  { key: 'Display_FieldOfView', label: 'Field of view (degrees)', group: 'display', type: 'number', default: '78.19', min: 1, max: 179, step: 0.01 },
  { key: 'Display_PitchFovCompensation', label: 'Pitch FOV compensation', group: 'display', type: 'onoff', default: 'on' },
  { key: 'Display_MouseCursorScale', label: 'Mouse cursor scale (0 = auto)', group: 'display', type: 'number', default: '0', min: 0 },

  { key: 'Mouse_Look', label: 'Mouse look', group: 'mouse', type: 'yesno', default: 'yes', phoneDefault: 'no' },
  { key: 'Mouse_LookSensitivity', label: 'Mouse look sensitivity (%)', group: 'mouse', type: 'number', default: '100', min: 1 },

  { key: 'Game_PopupDelay', label: 'Pop-up accept delay (ticks)', group: 'game', type: 'number', default: '10', min: 0 },
];

var Game_GodModeField = { key: 'Game_GodMode', label: 'God mode', group: 'game', type: 'onoff', default: 'off' };

var Game_PhoneOnlyOverrides = { Display_MouseCursor: 'none', Display_ScalerFactor: '3' };

var Game_LanguageField = {
  key: 'Language', label: 'Game language', group: 'language', type: 'select', default: 'ENGLISH',
  options: [['GERMAN', 'German'], ['ENGLISH', 'English'], ['FRENCH', 'French']],
  cookieName: 'alb_lang', isLanguage: true
};

var Game_LanguageNumbers = { GERMAN: 1, ENGLISH: 2, FRENCH: 3 };
function Game_LanguageToNumber(lang) {
  var n = Game_LanguageNumbers[String(lang).toUpperCase()];
  return n ? n : 2;
}

function Game_GetCookie(name) {
  var re = new RegExp('(?:^|; )' + name.replace(/([.$?*|{}()\[\]\\\/+^])/g, '\\$1') + '=([^;]*)');
  var m = document.cookie.match(re);
  return m ? decodeURIComponent(m[1]) : null;
}

function Game_SetCookie(name, value) {
  var maxAge = 10 * 365 * 24 * 60 * 60;
  document.cookie = name + '=' + encodeURIComponent(value) + '; path=/; max-age=' + maxAge + '; samesite=lax';
}

function Game_ForEachCookie(fn) {
  document.cookie.split(';').forEach(function (part) {
    part = part.replace(/^\s+|\s+$/g, '');
    if (!part) return;
    var eq = part.indexOf('=');
    if (eq === -1) return;
    fn(decodeURIComponent(part.substring(0, eq)), decodeURIComponent(part.substring(eq + 1)));
  });
}

function Game_FieldCookieName(field) {
  return field.cookieName || ('alb_cfg_' + field.key);
}

function Game_ParseFragmentParams() {
  var hash = location.hash;
  if (hash.length <= 1) return new URLSearchParams();
  var query = (hash.charAt(1) === '?') ? hash.substring(2) : hash.substring(1);
  return new URLSearchParams(query);
}

function Game_FieldUriValue(field, params) {
  if (field.isLanguage) {
    var v = null;
    params.forEach(function (value, key) { if (key.toLowerCase() === 'language') v = value; });
    return v;
  }
  return params.has(field.key) ? params.get(field.key) : null;
}

function Game_IniSetVariable(text, section, key, value) {
  var lines = text.split(/\r?\n/);
  var out = [];
  var inSection = false, sectionFound = false, keyFound = false;

  for (var i = 0; i < lines.length; i++) {
    var line = lines[i];
    var trimmed = line.replace(/^[ \t]+|[ \t]+$/g, '');

    if (trimmed.length > 1 && trimmed.charAt(0) === '[' && trimmed.charAt(trimmed.length - 1) === ']') {
      if (inSection && !keyFound) {
        out.push(key + '=' + value);
        keyFound = true;
      }
      var name = trimmed.substring(1, trimmed.length - 1);
      inSection = name.toUpperCase() === section.toUpperCase();
      if (inSection) sectionFound = true;
      out.push(line);
      continue;
    }

    if (inSection && !keyFound) {
      var eq = trimmed.indexOf('=');
      var lineKey = (eq === -1) ? trimmed : trimmed.substring(0, eq);
      if (lineKey.toUpperCase() === key.toUpperCase()) {
        out.push(key + '=' + value);
        keyFound = true;
        continue;
      }
    }

    out.push(line);
  }

  if (inSection && !keyFound) {
    out.push(key + '=' + value);
    keyFound = true;
  }

  if (!sectionFound) {
    if (out.length && out[out.length - 1] !== '') out.push('');
    out.push('[' + section + ']');
    out.push(key + '=' + value);
  }

  return out.join('\n');
}

function Game_ApplyBootOverrides() {
  var params = Game_ParseFragmentParams();
  var isPhone = (typeof Game_IsPhone === 'function') && Game_IsPhone();
  var allFields = Game_SettingsFields.concat([Game_GodModeField]);

  var cfgOverrides = '';

  if (isPhone) {
    Game_SettingsFields.forEach(function (field) {
      if (field.phoneDefault == null) return;
      if (params.has(field.key)) return;
      if (Game_GetCookie(Game_FieldCookieName(field)) != null) return;
      cfgOverrides += field.key + '=' + field.phoneDefault + '\n';
    });
    Object.keys(Game_PhoneOnlyOverrides).forEach(function (key) {
      if (params.has(key)) return;
      cfgOverrides += key + '=' + Game_PhoneOnlyOverrides[key] + '\n';
    });
  }

  allFields.forEach(function (field) {
    var c = Game_GetCookie(Game_FieldCookieName(field));
    if (c != null) cfgOverrides += field.key + '=' + c + '\n';
  });
  params.forEach(function (value, key) {
    if (key.toLowerCase() === 'language') return;
    cfgOverrides += key + '=' + value + '\n';
  });
  if (cfgOverrides) {
    var existingCfg = '';
    try { existingCfg = FS.readFile('/Albion.cfg', { encoding: 'utf8' }); } catch (e) {}
    FS.writeFile('/Albion.cfg', existingCfg + '\n' + cfgOverrides);
  }

  var setupText = null;
  try { setupText = FS.readFile('/SETUP.INI', { encoding: 'utf8' }); } catch (e) {}
  if (setupText != null) {
    var changed = false;

    var lang = Game_FieldUriValue(Game_LanguageField, params);
    if (lang == null) lang = Game_GetCookie('alb_lang');
    if (lang != null) {
      setupText = Game_IniSetVariable(setupText, 'SYSTEM', 'LANGUAGE', String(Game_LanguageToNumber(lang)));
      changed = true;
    }

    Game_ForEachCookie(function (name, value) {
      if (name.indexOf('alb_opt_') === 0) {
        setupText = Game_IniSetVariable(setupText, 'ALBION', name.substring('alb_opt_'.length), value);
        changed = true;
      }
    });

    if (changed) FS.writeFile('/SETUP.INI', setupText);
  }
}

window.Game_SaveSetupOptions = function (payload) {
  payload.split('\n').forEach(function (line) {
    line = line.replace(/^\s+|\s+$/g, '');
    if (!line) return;
    var eq = line.indexOf('=');
    if (eq === -1) return;
    var key = line.substring(0, eq).replace(/^\s+|\s+$/g, '');
    var value = line.substring(eq + 1).replace(/^\s+|\s+$/g, '');
    if (key === 'SAVED_GAME_NR' && value === '101')
    {
      value = '100';
    }
    if (key) Game_SetCookie('alb_opt_' + key, value);
  });
};

var Game_SettingsGroupLabels = { language: 'Language', audio: 'Audio', display: 'Display', mouse: 'Mouse look', game: 'Game tweaks' };
var Game_SettingsGroupOrder = ['language', 'audio', 'display', 'mouse', 'game'];

function Game_BuildFieldRow(field, params) {
  var row = document.createElement('label');
  row.className = 'settings-row';

  var span = document.createElement('span');
  span.className = 'settings-label';
  span.textContent = field.label;
  row.appendChild(span);

  var uriValue = Game_FieldUriValue(field, params);
  var cookieValue = Game_GetCookie(Game_FieldCookieName(field));
  var isPhone = (typeof Game_IsPhone === 'function') && Game_IsPhone();
  var phoneValue = (isPhone && field.phoneDefault != null) ? field.phoneDefault : null;
  var effective = (uriValue != null) ? uriValue : (cookieValue != null ? cookieValue : (phoneValue != null ? phoneValue : field.default));
  var forced = uriValue != null;

  var control;
  if (field.type === 'number') {
    control = document.createElement('input');
    control.type = 'number';
    if (field.min != null) control.min = field.min;
    if (field.max != null) control.max = field.max;
    if (field.step != null) control.step = field.step;
    control.value = effective;
    control.addEventListener('input', function () {
      if (control.value === '') return;
      Game_SetCookie(Game_FieldCookieName(field), control.value);
    });
  } else {
    control = document.createElement('select');

    var opts = field.options || (field.type === 'onoff' ? [['on', 'On'], ['off', 'Off']] : [['yes', 'Yes'], ['no', 'No']]);
    opts.forEach(function (pair) {
      var opt = document.createElement('option');
      opt.value = pair[0];
      opt.textContent = pair[1];
      control.appendChild(opt);
    });

    control.value = effective;
    control.addEventListener('change', function () {
      Game_SetCookie(Game_FieldCookieName(field), control.value);
    });
  }

  control.disabled = forced;
  row.appendChild(control);

  return row;
}

function Game_BuildSettingsPanel() {
  var panel = document.getElementById('settings-panel');
  if (!panel) return;
  panel.innerHTML = '';

  var params = Game_ParseFragmentParams();
  var byGroup = { language: [Game_LanguageField] };
  var fields = Game_SettingsFields.concat(params.has('Game_DeveloperMode') ? [Game_GodModeField] : []);
  fields.forEach(function (f) { (byGroup[f.group] = byGroup[f.group] || []).push(f); });

  Game_SettingsGroupOrder.forEach(function (g) {
    var fields = byGroup[g];
    if (!fields || !fields.length) return;

    var fieldset = document.createElement('fieldset');
    var legend = document.createElement('legend');
    legend.textContent = Game_SettingsGroupLabels[g];
    fieldset.appendChild(legend);

    fields.forEach(function (field) {
      fieldset.appendChild(Game_BuildFieldRow(field, params));
    });

    panel.appendChild(fieldset);
  });
}

var btnSettings = document.getElementById('btn-settings');
if (btnSettings) {
  btnSettings.addEventListener('click', Game_BuildSettingsPanel);
}

var btnStartForSettings = document.getElementById('btn-start');
if (btnStartForSettings) {
  btnStartForSettings.addEventListener('click', function () {
    if (typeof showPanel === 'function') showPanel('game');
    if (btnSettings) {
      btnSettings.disabled = true;
      btnSettings.style.display = 'none';
    }
  }, { once: true });
}
