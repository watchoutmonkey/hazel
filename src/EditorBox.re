open Tyxml_js;
open SemanticsCore;
type t = {
  e: UHExp.t,
  pp_view: unit,
  pp_view_dom: unit,
};

let string_insert = (s1, offset, s2) => {
  let prefix = String.sub(s1, 0, offset);
  let length = String.length(s1);
  let suffix = String.sub(s1, offset, length - offset);
  prefix ++ s2 ++ suffix;
};

let string_backspace = (s, offset, ctrlKey) => {
  let prefix = ctrlKey ? "" : String.sub(s, 0, offset - 1);
  let length = String.length(s);
  let suffix = String.sub(s, offset, length - offset);
  let offset' = ctrlKey ? 0 : offset - 1;
  (prefix ++ suffix, offset');
};

let string_delete = (s, offset, ctrlKey) => {
  let prefix = String.sub(s, 0, offset);
  let length = String.length(s);
  let suffix = ctrlKey ? "" : String.sub(s, offset + 1, length - offset - 1);
  (prefix ++ suffix, offset);
};

let side_of_str_offset = (s, offset) =>
  if (offset == 0) {
    Before;
  } else if (offset == String.length(s)) {
    After;
  } else {
    In(offset);
  };

exception InvalidExpression;
let view = (mk_html_cell, prefix, rev_path, model: Model.t, e: UHExp.t): t => {
  let cursor_info_rs = model.cursor_info_rs;
  let do_action = model.do_action;
  let kc = JSUtil.KeyCombo.key;

  let edit_state = (e, ty);
  let pp_view =
    Html5.div(
      ~a=
        Html5.[
          a_contenteditable(true),
          a_onkeypress(evt => {
            let charCode = Js.Optdef.get(evt##.charCode, () => assert(false));

            let key =
              Js.to_string(Js.Optdef.get(evt##.key, () => assert(false)));

            switch (int_of_string_opt(key)) {
            | Some(n) =>
              let cursor_info = React.S.value(cursor_info_rs);
              switch (cursor_info.form) {
              | ZExp.IsHole(_) =>
                Dom.preventDefault(evt);
                do_action(Action.Construct(Action.SLit(n, After)));
                true;
              | ZExp.IsNumLit =>
                let selection = Dom_html.window##getSelection;
                let anchorNode = selection##.anchorNode;
                let nodeValue =
                  Js.to_string(
                    Js.Opt.get(anchorNode##.nodeValue, () => assert(false)),
                  );
                let anchorOffset = selection##.anchorOffset;
                let newNodeValue =
                  string_insert(nodeValue, anchorOffset, key);
                Dom.preventDefault(evt);
                switch (int_of_string_opt(newNodeValue)) {
                | Some(new_n) =>
                  let new_side =
                    side_of_str_offset(newNodeValue, anchorOffset + 1);
                  do_action(Action.Construct(Action.SLit(new_n, new_side)));
                | None => ()
                };
                true;
              | _ =>
                Dom.preventDefault(evt);
                true;
              };
            | None =>
              if (charCode != 0
                  || String.equal(key, "Enter")
                  || String.equal(key, "Tab")) {
                Dom.preventDefault(evt);
                true;
              } else {
                true;
              }
            };
          }),
          a_onkeydown(evt => {
            /* TODO we should use a more general matches function */
            let key = JSUtil.get_key(evt);
            let is_backspace = key == kc(JSUtil.KeyCombos.backspace);
            let is_del = key == kc(JSUtil.KeyCombos.del);
            if (is_backspace || is_del) {
              let cursor_info = React.S.value(cursor_info_rs);
              switch (cursor_info.form) {
              | ZExp.IsNumLit =>
                let side = cursor_info.side;
                let is_Before =
                  switch (side) {
                  | Before => true
                  | _ => false
                  };
                let is_After =
                  switch (side) {
                  | After => true
                  | _ => false
                  };
                if (is_backspace && is_Before || is_del && is_After) {
                  Dom.preventDefault(evt);
                  false;
                } else {
                  let selection = Dom_html.window##getSelection;
                  Dom_html.stopPropagation(evt);
                  Dom.preventDefault(evt);
                  let anchorNode = selection##.anchorNode;
                  let anchorOffset = selection##.anchorOffset;
                  let nodeValue =
                    Js.to_string(
                      Js.Opt.get(anchorNode##.nodeValue, () => assert(false)),
                    );
                  let ctrlKey = Js.to_bool(evt##.ctrlKey);
                  let (nodeValue', anchorOffset') =
                    is_backspace ?
                      string_backspace(nodeValue, anchorOffset, ctrlKey) :
                      string_delete(nodeValue, anchorOffset, ctrlKey);
                  if (String.equal(nodeValue', "")) {
                    if (is_Before) {
                      do_action(Action.Delete);
                    } else {
                      do_action(Action.Backspace);
                    };
                  } else {
                    let n = int_of_string(nodeValue');
                    let side = side_of_str_offset(nodeValue', anchorOffset');
                    do_action(Action.Construct(Action.SLit(n, side)));
                  };
                  false;
                };
              | _ =>
                Dom.preventDefault(evt);
                false;
              };
            } else {
              true;
            };
          }),
          a_ondrop(evt => {
            Dom.preventDefault(evt);
            false;
          }),
        ],
      [View.of_hexp(mk_html_cell, palette_stuff, prefix, rev_path, e)],
    );

  let pp_view_dom = Tyxml_js.To_dom.of_div(pp_view);
  let preventDefault_handler = evt => {
    Dom.preventDefault(evt);
    ();
  };
  let _ =
    JSUtil.listen_to_t(
      Dom.Event.make("paste"),
      pp_view_dom,
      preventDefault_handler,
    );

  let _ =
    JSUtil.listen_to_t(
      Dom.Event.make("cut"),
      pp_view_dom,
      preventDefault_handler,
    );

  let edit_box = {edit_state_rs, e_rs, pp_view, pp_view_dom};
  ();
  /* TODO whatever calls this should wrap it in a div of class "ModelExp"
     Html5.(div(~a=[a_class(["ModelExp"])], [pp_view]));
     */
};
