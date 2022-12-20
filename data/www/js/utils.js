const hide = el => {
    el.classList.add('hidden');
};


const show = el => {
    el.classList.remove('hidden');
};

const showhide = (sids, mode, by) => {
    if(!sids) return;
    var ids = sids.trim().split(/\s+/);

    for(let i in ids) {
      var el = document.getElementById(ids[i]+"-group");
      showhideone(el, !mode, by);
    }
};

const showhideone = (el, mode, by) => {
  var state = el.getAttribute("data-hidden"); 
  var hidden = (state?state.trim().split(/\s+/):[]); 
  var index = hidden.indexOf(by);

  if(mode) {
    // we check if not already hidden and save the state if yes
    if(el.classList.contains("hidden")) {
      if(index<0) {
        hidden.push(by);
        el.setAttribute("data-hidden", hidden.join(" "));
      }
    }
    else {
      hide(el);
      if(index<0) {
        hidden.push(by);
        el.setAttribute("data-hidden", hidden.join(" "));
      }
    }
  }  
  else {
    if(index>=0) {
      hidden.splice(index,1);
      el.setAttribute("data-hidden", hidden.join(" "));
    }
    if(!hidden.length) {
      show(el);
    }
  }
}

const disable = el => {
    el.classList.add('disabled')
    el.disabled = true
};

const enable = el => {
    el.classList.remove('disabled')
    el.disabled = false
};

const createInputGroup = (field) => {
  var div_group = document.createElement("div"); 
  div_group.classList.add("input-group");
  div_group.setAttribute("id", field.id + "-group");
  if(field.simple) div_group.setAttribute("data-simple", "true");
  if(field.title) div_group.setAttribute("title", field.title);
  if(field.hidden && field.hidden == "true") {
    div_group.classList.add("hidden");
    div_group.setAttribute("data-hidden", "_hidden");
  }
  return div_group;
}

const createLabelFor = (field) => {
    var lbl = document.createElement("label");
    lbl.innerHTML = field.name;
    lbl.setAttribute("for", field.id);
    return lbl
};

const addRange = (parent, field) => {
    var div_group = createInputGroup(field);

    div_group.appendChild(createLabelFor(field));
    
    var div_min = document.createElement("div");
    div_min.classList.add("range-min");
    div_min.innerHTML = field.min_caption;
    div_group.appendChild(div_min);

    var input = document.createElement("input");
    input.setAttribute("type", "range");
    input.setAttribute("id", field.id);
    input.setAttribute("min", field.min_value);
    input.setAttribute("max", field.max_value);
    input.setAttribute("value", field.default_value);
    input.setAttribute("class", field.classes);
    if(field.show) input.setAttribute("data-show", field.show);
    if(field.hide) input.setAttribute("data-hide", field.hide);
    div_group.appendChild(input);

    var div_max = document.createElement("div");
    div_max.classList.add("range-max");
    div_max.innerHTML = field.max_caption;
    div_group.appendChild(div_max);

    parent.appendChild(div_group);
};


const addSwitch = (parent, field) => {
    var div_group = createInputGroup(field);
    div_group.appendChild(createLabelFor(field));

    var div_sw = document.createElement("div");
    div_sw.classList.add("switch");

    var input = document.createElement("input");
    input.setAttribute("id", field.id);
    input.setAttribute("type", "checkbox");
    input.setAttribute("class", field.classes);
    if(field.default_value) input.setAttribute("checked", "checked");
    if(field.show) input.setAttribute("data-show", field.show);
    if(field.hide) input.setAttribute("data-hide", field.hide);
    div_sw.appendChild(input);

    var lbl_slider = document.createElement("label");
    lbl_slider.classList.add("slider");
    lbl_slider.setAttribute("for", field.id);
    div_sw.appendChild(lbl_slider);

    div_group.appendChild(div_sw);

    parent.appendChild(div_group);

};

const addSelector = (parent, field, opts, optfilters) => {
    var div_group = createInputGroup(field);
    div_group.appendChild(createLabelFor(field));

    var sel = document.createElement("select");
    sel.setAttribute("id", field.id);
    sel.setAttribute("class", field.classes);

    var opt_defs = opts.find(item => item.field == field.id);
    if(opt_defs) {
      for(let i in opt_defs.options) {
        var opt_def = opt_defs.options[i];
        if(!isOptionFiltered(field, optfilters, opt_def)) {
          var opt = document.createElement("option");
          opt.setAttribute("value", opt_def.id);
          opt.innerHTML = opt_def.name;
          if(field.default_value && opt_def.id == field.default_value) {
            opt.setAttribute("selected", "selected");
          }
          sel.appendChild(opt);
        }
      }
    }
    else {
      console.log("Did not find options for "+field.id);
    }

    div_group.appendChild(sel);

    parent.appendChild(div_group);
};

const isOptionFiltered = (field, optfilters, opt_def) => {
  if(!optfilters) return false;
  
  if(optfilters.show) {
    var allowed = optfilters.show.find(item => item.field == field.id);
    if(allowed) {
      return !allowed.options.includes(opt_def.id);
    }
    else  
      return false;
  }
  if(optfilters.hide) {
    var forbidden = optfilters.hide.find(item => item.field == field.id);
    if(forbidden) {
      return forbidden.options.includes(opt_def.id);
    }
  }  
  return false;
  
};

const addTextInput = (parent, field) => {
    var div_group = createInputGroup(field);
    div_group.appendChild(createLabelFor(field));

    var div_txt = document.createElement("div");
    div_txt.classList.add("text");

    var input = document.createElement("input");
    input.setAttribute("id", field.id);
    if(field.size) input.setAttribute("size", field.size);
    if(field.classes) input.setAttribute("class", field.classes);
    if(field.type == "number") {
      input.setAttribute("type", "number");
      if(field.min_value) input.setAttribute("min", field.min_value);
      if(field.max_value) input.setAttribute("max", field.max_value);
      if(field.step) input.setAttribute("step", field.step);
    }
    else if(field.type == "ipv4") {
      input.setAttribute("type", "text");
      input.setAttribute("placeholder", "xxx.xxx.xxx.xxx");
      input.classList.add("ipv4");
    }
    else {
      input.setAttribute("type", field.type);
    }
    div_txt.appendChild(input);

    if(field.max_caption) {
        var lbl_max = document.createElement("div");
        lbl_max.classList.add("range-max");
        lbl_max.innerHTML = field.max_caption;
        div_txt.appendChild(lbl_max);
    }

    div_group.appendChild(div_txt);

    parent.appendChild(div_group);
};

const addReadOnly = (parent, field) => {
  var div_group = createInputGroup(field);
  div_group.appendChild(createLabelFor(field));

  var div_val = document.createElement("div");
  div_val.setAttribute("id", field.id);
  div_val.setAttribute("class", field.classes);
  div_group.appendChild(div_val);

  parent.appendChild(div_group);
};



const createButton = (id, name, title, confirm, val) => {
    var btn = document.createElement("button");
    btn.setAttribute("id", id);
    btn.setAttribute("title", title);
    btn.setAttribute("class", "default-action");
    btn.setAttribute("data-ask", confirm)
    btn.setAttribute("data-value", val);
    btn.innerHTML = name;

    return btn;
};

const addFormFields = (parent, fields, opts, optfilters) => {
    // console.log("adding form fields");
    for(let i in fields) {
      var field = fields[i];

      switch(field.control) {
        case "readonly": 
          addReadOnly(parent, field);
          break;
        case "switch":
          addSwitch(parent, field);
          break;
        case "range":
          addRange(parent, field);
          break;
        case "select":
          addSelector(parent, field, opts, optfilters);
          break;
        default:
          addTextInput(parent, field);
          break;
      }
    }
    
};

const updateFormFieldDefs = (fields, fieldupdates) => {
  if(!fieldupdates) return;

  if(fieldupdates.update) {
    let updates = fieldupdates.update;
    for(let i in updates) {
      var index = fields.findIndex((field) => field.id == updates[i].id);
      if(index >= 0 ) {
        var changes = updates[i].changes
        for(const key of Object.keys(changes)) 
          fields[index][key] = changes[key];
      }
    }
  }

  if(fieldupdates.create) {
    let creates = fieldupdates.create;
    for(let i in creates) {
      var index = fields.findIndex((field) => field.id == creates[i].after);
      fields.splice(index+1, 0, creates[i]);
    }
  }
};

function refreshControl(el) {
  let value;
  switch (el.type) {
    case 'checkbox':
      value = el.checked ? 1 : 0;
      showhide(el.getAttribute("data-show"), value, el.id);
      showhide(el.getAttribute("data-hide"), !value, el.id);
      break;
    case 'range':
      value = el.value;
      var isshown = el.getAttribute("data-outofrange") != "true";
      showhide(el.getAttribute("data-show"), isshown, el.id);
      showhide(el.getAttribute("data-hide"), !isshown, el.id);
      break;
    case 'select-one':
    case 'number':
    case 'text':
    case 'password':  
      value = el.value;
      break;
    case 'button':
    case 'submit':
      value = el.getAttribute("data-value");
      break;
    default:
      if(el.nodeName == "DIV") {
        el.innerHTML = el.value;
      }
      break;
    } 
    return value;
}

const loadControlValue = (el, value) => {

  let initialValue;
  if (el.type === 'checkbox') {
    initialValue = el.checked;
    value = !!value;
    el.checked = value;
  } else {
    initialValue = el.value;
    el.value = value;
    if(el.value != value) {
      el.setAttribute("data-outofrange", "true");
    }
  }
}


function updateRangeConfig (el) {
  if (!el.getAttribute("data-updating") == "true") {
    el.setAttribute("data-updating", "true");
    setTimeout(function(el){
      submitChanges(el);
    }, 150, el);
  }
}

function submitChanges (el) {

  if(el.type == "submit") {
    if(el.getAttribute("data-ask")) {
      if(!confirm(el.getAttribute("title") + "?")) return false;
    }

    if(el.id == "reboot") {
      setTimeout(function() {
        location.replace(document.URL);
      }, 30000);
    } 
  }
  else if(el.type == "range") {
    el.setAttribute("data-updating", "");
  }

  let host = document.location.origin;
  let value = refreshControl(el);

  const query = `${host}/control?var=${el.id}&val=${value}`;

  fetch(query)
    .then(response => {
      console.log(`request to ${query} finished, status: ${response.status}`);
    });
  
    return true;
}



