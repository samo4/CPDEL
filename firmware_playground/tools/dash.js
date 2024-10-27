var fr = Object.defineProperty, dr = Object.defineProperties;
var ur = Object.getOwnPropertyDescriptors;
var Tn = Object.getOwnPropertySymbols;
var gr = Object.prototype.hasOwnProperty, pr = Object.prototype.propertyIsEnumerable;
var En = (i, t, e) => t in i ? fr(i, t, {enumerable: true, configurable: true, writable: true, value: e}) : i[t] = e, Ei = (i, t) => {
  for (var e in t || (t = {})) gr.call(t, e) && En(i, e, t[e]);
  if (Tn) for (var e of Tn(t)) pr.call(t, e) && En(i, e, t[e]);
  return i;
}, Ri = (i, t) => dr(i, ur(t));
const mr = function () {
  const t = document.createElement("link").relList;
  if (t && t.supports && t.supports("modulepreload")) return;
  for (const s of document.querySelectorAll('link[rel="modulepreload"]')) n(s);
  new MutationObserver(s => {
    for (const o of s) if (o.type === "childList") for (const r of o.addedNodes) r.tagName === "LINK" && r.rel === "modulepreload" && n(r);
  }).observe(document, {childList: true, subtree: true});
  function e(s) {
    const o = {};
    return s.integrity && (o.integrity = s.integrity), s.referrerpolicy && (o.referrerPolicy = s.referrerpolicy), s.crossorigin === "use-credentials" ? o.credentials = "include" : s.crossorigin === "anonymous" ? o.credentials = "omit" : o.credentials = "same-origin", o;
  }
  function n(s) {
    if (s.ep) return;
    s.ep = true;
    const o = e(s);
    fetch(s.href, o);
  }
};
mr();
function gt() {}
function Jt(i, t) {
  for (const e in t) i[e] = t[e];
  return i;
}
function ee(i) {
  i.forEach(fo);
}
function Ht(i, t) {
  return i != i ? t == t : i !== t || i && typeof i == "object" || typeof i == "function";
}
function xr(i, t, e, n) {
  if (i) {
    const s = uo(i, t, e, n);
    return i[0](s);
  }
}
function uo(i, t, e, n) {
  return i[1] && n ? Jt(e.ctx.slice(), i[1](n(t))) : e.ctx;
}
function yr(i, t, e, n) {
  if (i[2] && n) {
    const s = i[2](n(e));
    if (t.dirty === undefined) return s;
    if (typeof s == "object") {
      const o = [], r = Math.max(t.dirty.length, s.length);
      for (let a = 0; a < r; a += 1) o[a] = t.dirty[a] | s[a];
      return o;
    }
    return t.dirty | s;
  }
  return t.dirty;
}
function vr(i, t, e, n, s, o) {
  if (s) {
    const r = uo(t, e, n, o);
    i.p(r, s);
  }
}
function wr(i) {
  if (i.ctx.length > 32) {
    const t = [], e = i.ctx.length / 32;
    for (let n = 0; n < e; n++) t[n] = -1;
    return t;
  }
  return -1;
}
function ei(i) {
  const t = {};
  for (const e in i) e[0] !== "$" && (t[e] = i[e]);
  return t;
}
function M(i, t) {
  i.appendChild(t);
}
function O(i, t, e) {
  i.insertBefore(t, e || null);
}
function D(i) {
  i.parentNode.removeChild(i);
}
function g(i, t, e) {
  e == null ? i.removeAttribute(t) : i.getAttribute(t) !== e && i.setAttribute(t, e);
}
function Bn(i, t) {
  const e = Object.getOwnPropertyDescriptors(i.__proto__);
  for (const n in t) t[n] == null ? i.removeAttribute(n) : n === "style" ? i.style.cssText = t[n] : n === "__value" ? i.value = i[n] = t[n] : e[n] && e[n].set ? i[n] = t[n] : g(i, n, t[n]);
}
function kr(i) {
  return i === "" ? null : +i;
}
function ut(i, t) {
  t = "" + t, i.wholeText !== t && (i.data = t);
}
function In(i, t) {
  i.value = t == null ? "" : t;
}
function Ar(i, t, {bubbles: e = false, cancelable: n = false} = {}) {
  const s = document.createEvent("CustomEvent");
  return s.initCustomEvent(i, e, n, t), s;
}
let Ce;
function ve(i) {
  Ce = i;
}
function _i() {
  if (!Ce) throw new Error("Function called outside component initialization");
  return Ce;
}
function hn(i) {
  _i().$$.on_mount.push(i);
}
function Sr(i) {
  _i().$$.after_update.push(i);
}
function Cr(i) {
  _i().$$.on_destroy.push(i);
}
function fn() {
  const i = _i();
  return (t, e, {cancelable: n = false} = {}) => {
    const s = i.$$.callbacks[t];
    if (s) {
      const o = Ar(t, e, {cancelable: n});
      return s.slice().forEach(r => {
        r.call(i, o);
      }), !o.defaultPrevented;
    }
    return true;
  };
}
const me = [], Ki = [], Ze = [], zn = [], Pr = Promise.resolve();
let Xi = false;
function Dr() {
  Xi || (Xi = true, Pr.then(go));
}
function Ji(i) {
  Ze.push(i);
}
const Bi = new Set;
let He = 0;
function go() {
  const i = Ce;
  do {
    for (; He < me.length;) {
      const t = me[He];
      He++, ve(t), Or(t.$$);
    }
    for (ve(null), me.length = 0, He = 0; Ki.length;) Ki.pop()();
    for (let t = 0; t < Ze.length; t += 1) {
      const e = Ze[t];
      Bi.has(e) || (Bi.add(e), e());
    }
    Ze.length = 0;
  } while (me.length);
  for (; zn.length;) zn.pop()();
  Xi = false, Bi.clear(), ve(i);
}
function Or(i) {
  if (i.fragment !== null) {
    i.update(), ee(i.before_update);
    const t = i.dirty;
    i.dirty = [-1], i.fragment && i.fragment.p(i.ctx, t), i.after_update.forEach(Ji);
  }
}
const $e = new Set;
let Kt;
function ii() {
  Kt = {r: 0, c: [], p: Kt};
}
function ni() {
  Kt.r || ee(Kt.c), Kt = Kt.p;
}
function et(i, t) {
  i && i.i && ($e.delete(i), i.i(t));
}
function rt(i, t, e, n) {
  if (i && i.o) {
    if ($e.has(i)) return;
    $e.add(i), Kt.c.push(() => {
      $e.delete(i), n && (e && i.d(1), n());
    }), i.o(t);
  }
}
function Lr(i, t) {
  i.d(1), t.delete(i.key);
}
function Fn(i, t) {
  rt(i, 1, 1, () => {
    t.delete(i.key);
  });
}
function qi(i, t, e, n, s, o, r, a, l, c, h, f) {
  let d = i.length, u = o.length, p = d;
  const m = {};
  for (; p--;) m[i[p].key] = p;
  const b = [], x = new Map, _ = new Map;
  for (p = u; p--;) {
    const y = f(s, o, p), A = e(y);
    let C = r.get(A);
    C ? n && C.p(y, t) : (C = c(A, y), C.c()), x.set(A, b[p] = C), A in m && _.set(A, Math.abs(p - m[A]));
  }
  const w = new Set, k = new Set;
  function v(y) {
    et(y, 1), y.m(a, h), r.set(y.key, y), h = y.first, u--;
  }
  for (; d && u;) {
    const y = b[u - 1], A = i[d - 1], C = y.key, P = A.key;
    y === A ? (h = y.first, d--, u--) : x.has(P) ? !r.has(C) || w.has(C) ? v(y) : k.has(P) ? d-- : _.get(C) > _.get(P) ? (k.add(C), v(y)) : (w.add(P), d--) : (l(A, r), d--);
  }
  for (; d--;) {
    const y = i[d];
    x.has(y.key) || l(y, r);
  }
  for (; u;) v(b[u - 1]);
  return b;
}
function po(i, t) {
  const e = {}, n = {}, s = {$$scope: 1};
  let o = i.length;
  for (; o--;) {
    const r = i[o], a = t[o];
    if (a) {
      for (const l in r) l in a || (n[l] = 1);
      for (const l in a) s[l] || (e[l] = a[l], s[l] = 1);
      i[o] = a;
    } else for (const l in r) s[l] = 1;
  }
  for (const r in n) r in e || (e[r] = undefined);
  return e;
}
function Tr(i) {
  return typeof i == "object" && i !== null ? i : {};
}
function jt(i) {
  i && i.c();
}
function Tt(i, t, e, n) {
  const {fragment: s, on_mount: o, on_destroy: r, after_update: a} = i.$$;
  s && s.m(t, e), n || Ji(() => {
    const l = o.map(fo).filter(br);
    r ? r.push(...l) : ee(l), i.$$.on_mount = [];
  }), a.forEach(Ji);
}
function Et(i, t) {
  const e = i.$$;
  e.fragment !== null && (ee(e.on_destroy), e.fragment && e.fragment.d(t), e.on_destroy = e.fragment = null, e.ctx = []);
}
function Er(i, t) {
  i.$$.dirty[0] === -1 && (me.push(i), Dr(), i.$$.dirty.fill(0)), i.$$.dirty[t / 31 | 0] |= 1 << t % 31;
}
function Wt(i, t, e, n, s, o, r, a = [-1]) {
  const l = Ce;
  ve(i);
  const c = i.$$ = {fragment: null, ctx: null, props: o, update: gt, not_equal: s, bound: Object.create(null), on_mount: [], on_destroy: [], on_disconnect: [], before_update: [], after_update: [], context: new Map(t.context || (l ? l.$$.context : [])), callbacks: Object.create(null), dirty: a, skip_bound: false, root: t.target || l.$$.root};
  r && r(c.root);
  let h = false;
  if (c.ctx = e ? e(i, t.props || {}, (f, d, ...u) => {
    const p = u.length ? u[0] : d;
    return c.ctx && s(c.ctx[f], c.ctx[f] = p) && (!c.skip_bound && c.bound[f] && c.bound[f](p), h && Er(i, f)), d;
  }) : [], c.update(), h = true, ee(c.before_update), c.fragment = n ? n(c.ctx) : false, t.target) {
    if (t.hydrate) {
      const f = Array.from(t.target.childNodes);
      c.fragment && c.fragment.l(f), f.forEach(D);
    } else c.fragment && c.fragment.c();
    t.intro && et(i.$$.fragment), Tt(i, t.target, t.anchor, t.customElement), go();
  }
  ve(l);
}
class Nt {
  $destroy() {
    Et(this, 1), this.$destroy = gt;
  }
  $on(t, e) {
    const n = this.$$.callbacks[t] || (this.$$.callbacks[t] = []);
    return n.push(e), () => {
      const s = n.indexOf(e);
      s !== -1 && n.splice(s, 1);
    };
  }
  $set(t) {
    this.$$set && !(Object.keys(t).length === 0) && (this.$$.skip_bound = true, this.$$set(t), this.$$.skip_bound = false);
  }
}
function Rr(i) {
  let t, e, n, s, o;
  const r = i[4].default, a = xr(r, i, i[3], null);
  return {c() {
    t = document.createElement("button"), a && a.c(), g(t, "class", e = `flex flex-row gap-4 font-semibold tracking-wide w-full border-2 border-transparent ${i[1] ? "bg-blueish-blue border-blueish-blue text-white" : "bg-white text-[#6b6b6b] hover:bg-gray-50 hover:text-blueish-blue"} items-center px-4 py-2.5 ${i[0]}`);
  }, m(l, c) {
    O(l, t, c), a && a.m(t, null), n = true, s || (o = (t.addEventListener("click", i[2], n), () => t.removeEventListener("click", i[2], n)), s = true);
  }, p(l, [c]) {
    a && a.p && (!n || c & 8) && vr(a, r, l, l[3], n ? yr(r, l[3], c, null) : wr(l[3]), null), (!n || c & 3 && e !== (e = `flex flex-row gap-4 font-semibold tracking-wide w-full border-2 border-transparent ${l[1] ? "bg-blueish-blue border-blueish-blue text-white" : "bg-white text-[#6b6b6b] hover:bg-gray-50 hover:text-blueish-blue"} items-center px-4 py-2.5 ${l[0]}`)) && g(t, "class", e);
  }, i(l) {
    n || (et(a, l), n = true);
  }, o(l) {
    rt(a, l), n = false;
  }, d(l) {
    l && D(t), a && a.d(l), s = false, o();
  }};
}
function Br(i, t, e) {
  let {$$slots: n = {}, $$scope: s} = t;
  const o = fn();
  let {className: r} = t, {active: a} = t;
  const l = c => {
    o(c.type, c);
  };
  return i.$$set = c => {
    "className" in c && e(0, r = c.className), "active" in c && e(1, a = c.active), "$$scope" in c && e(3, s = c.$$scope);
  }, [r, a, l, s, n];
}
class mo extends Nt {
  constructor(t) {
    super(), Wt(this, t, Br, Rr, Ht, {className: 0, active: 1});
  }
}
function Ir(i) {
  let t, e, n;
  return {c() {
    t = document.createElementNS("http://www.w3.org/2000/svg", "svg"), e = document.createElementNS("http://www.w3.org/2000/svg", "path"), n = document.createElementNS("http://www.w3.org/2000/svg", "path"), g(e, "fill", "none"), g(e, "d", "M0 0h24v24H0z"), g(n, "d", "M10.828 12l4.95 4.95-1.414 1.414L8 12l6.364-6.364 1.414 1.414z"), g(t, "xmlns", "http://www.w3.org/2000/svg"), g(t, "viewBox", "0 0 24 24"), g(t, "width", "20"), g(t, "height", "20"), g(t, "fill", "currentColor");
  }, m(s, o) {
    O(s, t, o), M(t, e), M(t, n);
  }, d(s) {
    s && D(t);
  }};
}
function zr(i) {
  let t, e, n;
  return {c() {
    t = document.createElementNS("http://www.w3.org/2000/svg", "svg"), e = document.createElementNS("http://www.w3.org/2000/svg", "path"), n = document.createElementNS("http://www.w3.org/2000/svg", "path"), g(e, "fill", "none"), g(e, "d", "M0 0h24v24H0z"), g(n, "d", "M13.172 12l-4.95-4.95 1.414-1.414L16 12l-6.364 6.364-1.414-1.414z"), g(t, "xmlns", "http://www.w3.org/2000/svg"), g(t, "viewBox", "0 0 24 24"), g(t, "width", "20"), g(t, "height", "20"), g(t, "fill", "currentColor");
  }, m(s, o) {
    O(s, t, o), M(t, e), M(t, n);
  }, d(s) {
    s && D(t);
  }};
}
function Fr(i) {
  let t, e, n;
  return {c() {
    t = document.createElementNS("http://www.w3.org/2000/svg", "svg"), e = document.createElementNS("http://www.w3.org/2000/svg", "style"), n = document.createElementNS("http://www.w3.org/2000/svg", "path"), g(n, "d", "m0.8 1.9h35q17.9 0 29.8 12.4 12 12.3 12 30.7 0 18.3-12 30.7-11.9 12.3-29.8 12.3h-35zm22.6 64.8h12.4q9 0 14.7-5.9 5.6-6 5.6-15.8 0-9.9-5.6-15.8-5.7-6-14.7-6h-12.4zm137.6 21.3h-23.7l-3.7-12.5h-28.7l-3.6 12.5h-23.9l28.4-86.1h26.8zm-41.8-60.7l-8.6 29.3h17.3zm76.9 62.5q-13.6 0-22.6-5.8-9-5.8-12.6-15.6l18.9-11q4.7 11.5 16.9 11.5 10.1 0 10.1-5.8 0-3.8-5.8-6.2-2.4-0.9-10.8-3.3-11.8-3.4-18.7-9.6-6.9-6.3-6.9-17.3 0-12 8.5-19.3 8.6-7.4 21.4-7.4 10.8 0 19.2 5 8.4 5.1 12.8 14.7l-18.5 10.8q-4-9.6-13.4-9.6-3.9 0-6 1.6-2 1.6-2 4.1 0 2.8 3 4.8 3.1 1.9 11.8 4.5 6.3 1.8 10.1 3.4 3.8 1.6 8.3 4.7 4.6 3 6.8 7.8 2.2 4.6 2.2 11 0 12.7-9 19.8-8.9 7.2-23.7 7.2zm87-56.1v-31.9h21.9v86.1h-21.9v-33.1h-24.3v33.1h-22v-86.1h22v31.9z"), g(t, "xmlns", "http://www.w3.org/2000/svg"), g(t, "viewBox", "0 0 305 90"), g(t, "class", "w-[80px] md:w-[110px]");
  }, m(s, o) {
    O(s, t, o), M(t, e), M(t, n);
  }, d(s) {
    s && D(t);
  }};
}
function Hr(i) {
  let t, e, n;
  return {c() {
    t = document.createElementNS("http://www.w3.org/2000/svg", "svg"), e = document.createElementNS("http://www.w3.org/2000/svg", "style"), n = document.createElementNS("http://www.w3.org/2000/svg", "path"), g(n, "d", "m0.5 0h43.5q21.8 0 36.3 15.2 14.7 15 14.7 37.3 0 22.3-14.7 37.5-14.5 15-36.3 15h-43.5zm31.5 75.9h12q9.1 0 15-6.4 6-6.7 6-17 0-10.3-6-16.8-5.9-6.6-15-6.6h-12z"), g(t, "xmlns", "http://www.w3.org/2000/svg"), g(t, "viewBox", "0 0 95 105"), g(t, "class", "w-[32px]");
  }, m(s, o) {
    O(s, t, o), M(t, e), M(t, n);
  }, d(s) {
    s && D(t);
  }};
}
function jr(i) {
  let t, e, n, s, o, r, a;
  return {c() {
    t = document.createElementNS("http://www.w3.org/2000/svg", "svg"), e = document.createElementNS("http://www.w3.org/2000/svg", "path"), n = document.createElementNS("http://www.w3.org/2000/svg", "path"), s = document.createTextNode(" "), o = document.createElement("span"), r = document.createTextNode("Overview"), g(e, "fill", "none"), g(e, "d", "M0 0h24v24H0z"), g(n, "fill", "currentColor"), g(n, "opacity", 0.4), g(n, "d", "M21 20a1 1 0 0 1-1 1H4a1 1 0 0 1-1-1V9.49a1 1 0 0 1 .386-.79l8-6.222a1 1 0 0 1 1.228 0l8 6.222a1 1 0 0 1 .386.79V20z"), g(t, "xmlns", "http://www.w3.org/2000/svg"), g(t, "viewBox", "0 0 24 24"), g(t, "width", "20"), g(t, "height", "20"), g(o, "class", a = i[3] ? "md:hidden" : "");
  }, m(l, c) {
    O(l, t, c), M(t, e), M(t, n), O(l, s, c), O(l, o, c), M(o, r);
  }, p(l, c) {
    c & 8 && a !== (a = l[3] ? "md:hidden" : "") && g(o, "class", a);
  }, d(l) {
    l && D(t), l && D(s), l && D(o);
  }};
}
function Hn(i) {
  let t, e;
  return t = new mo({props: {active: i[0] === "statistics", className: `rounded-lg ${i[3] ? "md:justify-center" : ""}`, $$slots: {default: [Wr]}, $$scope: {ctx: i}}}), t.$on("click", i[8]), {c() {
    jt(t.$$.fragment);
  }, m(n, s) {
    Tt(t, n, s), e = true;
  }, p(n, s) {
    const o = {};
    s & 1 && (o.active = n[0] === "statistics"), s & 8 && (o.className = `rounded-lg ${n[3] ? "md:justify-center" : ""}`), s & 1032 && (o.$$scope = {dirty: s, ctx: n}), t.$set(o);
  }, i(n) {
    e || (et(t.$$.fragment, n), e = true);
  }, o(n) {
    rt(t.$$.fragment, n), e = false;
  }, d(n) {
    Et(t, n);
  }};
}
function Wr(i) {
  let t, e, n, s, o, r, a;
  return {c() {
    t = document.createElementNS("http://www.w3.org/2000/svg", "svg"), e = document.createElementNS("http://www.w3.org/2000/svg", "path"), n = document.createElementNS("http://www.w3.org/2000/svg", "path"), s = document.createTextNode(" "), o = document.createElement("span"), r = document.createTextNode("Statistics"), g(e, "fill", "none"), g(e, "d", "M0 0h24v24H0z"), g(n, "fill", "currentColor"), g(n, "opacity", 0.4), g(n, "d", "M20.083 10.5l1.202.721a.5.5 0 0 1 0 .858L12 17.65l-9.285-5.571a.5.5 0 0 1 0-.858l1.202-.721L12 15.35l8.083-4.85zm0 4.7l1.202.721a.5.5 0 0 1 0 .858l-8.77 5.262a1 1 0 0 1-1.03 0l-8.77-5.262a.5.5 0 0 1 0-.858l1.202-.721L12 20.05l8.083-4.85zM12.514 1.309l8.771 5.262a.5.5 0 0 1 0 .858L12 13 2.715 7.429a.5.5 0 0 1 0-.858l8.77-5.262a1 1 0 0 1 1.03 0z"), g(t, "xmlns", "http://www.w3.org/2000/svg"), g(t, "viewBox", "0 0 24 24"), g(t, "width", "22"), g(t, "height", "22"), g(o, "class", a = i[3] ? "md:hidden" : "");
  }, m(l, c) {
    O(l, t, c), M(t, e), M(t, n), O(l, s, c), O(l, o, c), M(o, r);
  }, p(l, c) {
    c & 8 && a !== (a = l[3] ? "md:hidden" : "") && g(o, "class", a);
  }, d(l) {
    l && D(t), l && D(s), l && D(o);
  }};
}
function Nr(i) {
  let t;
  return {c() {
    t = document.createElement("a"), t.innerHTML = '<svg width="24" height="24" viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg"><g fill="none" fill-rule="evenodd"><path d="M0 0h24v24H0z"></path><path d="M12.374 19.939l5.849-8.773A.75.75 0 0017.599 10H13V4.477a.75.75 0 00-1.374-.416l-5.849 8.773A.75.75 0 006.401 14H11v5.523a.75.75 0 001.374.416z" fill="currentColor"></path></g></svg>', g(t, "href", "https://espdash.pro"), g(t, "target", "_blank"), g(t, "class", "py-3 px-3 flex flex-row justify-center items-center bg-black hover:bg-blueish-blue rounded-lg transition-colors font-semibold text-white");
  }, m(e, n) {
    O(e, t, n);
  }, d(e) {
    e && D(t);
  }};
}
function Vr(i) {
  let t;
  return {c() {
    t = document.createElement("div"), t.innerHTML = `<div class="py-5 relative"></div>`, g(t, "class", "flex flex-col gap-5 bg-[#f2f2f2] bg-opacity-60 rounded-2xl text-center p-4");
  }, m(e, n) {
    O(e, t, n);
  }, d(e) {
    e && D(t);
  }};
}
function Yr(i) {
  let t, e, n, s, o;
  return {c() {
    t = document.createElement("div"), e = document.createTextNode(" "), n = document.createElement("span"), s = document.createTextNode("Disconnected"), g(t, "class", "w-2 h-2 bg-red-500 rounded-full"), g(n, "class", o = i[3] ? "md:hidden" : "");
  }, m(r, a) {
    O(r, t, a), O(r, e, a), O(r, n, a), M(n, s);
  }, p(r, a) {
    a & 8 && o !== (o = r[3] ? "md:hidden" : "") && g(n, "class", o);
  }, d(r) {
    r && D(t), r && D(e), r && D(n);
  }};
}
function Ur(i) {
  let t, e, n, s, o;
  return {c() {
    t = document.createElement("div"), e = document.createTextNode(" "), n = document.createElement("span"), s = document.createTextNode("Connected"), g(t, "class", "w-2 h-2 bg-green-500 rounded-full"), g(n, "class", o = i[3] ? "md:hidden" : "");
  }, m(r, a) {
    O(r, t, a), O(r, e, a), O(r, n, a), M(n, s);
  }, p(r, a) {
    a & 8 && o !== (o = r[3] ? "md:hidden" : "") && g(n, "class", o);
  }, d(r) {
    r && D(t), r && D(e), r && D(n);
  }};
}
function Qr(i) {
  let t, e, n, s, o, r, a, l, c, h, f, d, u, p, m, b, x, _, w, k, v, y, A, C, P, H, I;
  function B(R, nt) {
    return R[3] ? zr : Ir;
  }
  let K = B(i), X = K(i);
  function E(R, nt) {
    return R[3] ? Hr : Fr;
  }
  let j = E(i), ct = j(i);
  p = new mo({props: {active: i[0] === "overview", className: `rounded-lg ${i[3] ? "md:justify-center" : ""}`, $$slots: {default: [jr]}, $$scope: {ctx: i}}}), p.$on("click", i[7]);
  let U = i[2] && Hn(i);
  function Rt(R, nt) {
    return R[3] ? Nr : Vr;
  }
  let At = Rt(i), lt = At(i);
  function Bt(R, nt) {
    return R[1] ? Ur : Yr;
  }
  let vt = Bt(i), ht = vt(i);
  return {c() {
    t = document.createElement("div"), e = document.createElement("button"), X.c(), n = document.createTextNode(" "), s = document.createElement("div"), ct.c(), o = document.createTextNode(" "), r = document.createElement("div"), a = document.createElementNS("http://www.w3.org/2000/svg", "svg"), l = document.createElementNS("http://www.w3.org/2000/svg", "style"), c = document.createElementNS("http://www.w3.org/2000/svg", "path"), h = document.createTextNode(" "), f = document.createElement("button"), f.innerHTML = '<svg class="w-6 h-6" viewBox="0 0 20 20" fill="currentColor"><path fill-rule="evenodd" d="M3 5a1 1 0 011-1h12a1 1 0 110 2H4a1 1 0 01-1-1zM3 10a1 1 0 011-1h12a1 1 0 110 2H4a1 1 0 01-1-1zM9 15a1 1 0 011-1h6a1 1 0 110 2h-6a1 1 0 01-1-1z" clip-rule="evenodd"></path></svg>', d = document.createTextNode(" "), u = document.createElement("div"), jt(p.$$.fragment), m = document.createTextNode(" "), U && U.c(), x = document.createTextNode(" "), _ = document.createElement("div"), lt.c(), k = document.createTextNode(" "), v = document.createElement("div"), y = document.createElement("div"), ht.c(), g(e, "class", "hidden absolute top-12 -right-5 w-5 h-10 hover:w-6 hover:-right-6 md:flex flex-col items-center transition-all ease-linear justify-center rounded-tr rounded-br shadow text-white bg-blueish-blue"), g(s, "class", "md:flex md:flex-col justify-center items-center shrink-0 h-36 mx-5 gap-8 hidden relative"), g(c, "d", "m0.8 1.9h35q17.9 0 29.8 12.4 12 12.3 12 30.7 0 18.3-12 30.7-11.9 12.3-29.8 12.3h-35zm22.6 64.8h12.4q9 0 14.7-5.9 5.6-6 5.6-15.8 0-9.9-5.6-15.8-5.7-6-14.7-6h-12.4zm137.6 21.3h-23.7l-3.7-12.5h-28.7l-3.6 12.5h-23.9l28.4-86.1h26.8zm-41.8-60.7l-8.6 29.3h17.3zm76.9 62.5q-13.6 0-22.6-5.8-9-5.8-12.6-15.6l18.9-11q4.7 11.5 16.9 11.5 10.1 0 10.1-5.8 0-3.8-5.8-6.2-2.4-0.9-10.8-3.3-11.8-3.4-18.7-9.6-6.9-6.3-6.9-17.3 0-12 8.5-19.3 8.6-7.4 21.4-7.4 10.8 0 19.2 5 8.4 5.1 12.8 14.7l-18.5 10.8q-4-9.6-13.4-9.6-3.9 0-6 1.6-2 1.6-2 4.1 0 2.8 3 4.8 3.1 1.9 11.8 4.5 6.3 1.8 10.1 3.4 3.8 1.6 8.3 4.7 4.6 3 6.8 7.8 2.2 4.6 2.2 11 0 12.7-9 19.8-8.9 7.2-23.7 7.2zm87-56.1v-31.9h21.9v86.1h-21.9v-33.1h-24.3v33.1h-22v-86.1h22v31.9z"), g(a, "xmlns", "http://www.w3.org/2000/svg"), g(a, "viewBox", "0 0 305 90"), g(a, "class", "w-[80px] md:w-[110px]"), g(f, "class", "flex items-center justify-center text-gray-500 w-8 h-8 focus:outline-none focus:shadow-outline relative md:order-first"), g(r, "class", "flex flex-row md:hidden justify-between gap-8 items-center mx-5 h-24"), g(u, "class", b = `${i[3] ? "hidden md:flex" : ""} flex flex-col flex-grow shrink-0 overflow-y-auto gap-4 mx-5`), g(_, "class", w = `${i[3] ? "hidden md:flex" : ""} flex flex-col justify-end gap-4 m-5 mt-10`), g(y, "class", "flex flex-row items-center px-4 py-2 gap-2 bg-gray-50 rounded-full text-sm text-gray-500 border border-gray-200"), g(v, "class", A = `${i[3] ? "hidden md:flex" : ""} flex flex-row justify-center gap-4 mb-5 mx-5`), g(t, "class", C = `flex flex-col ${i[3] ? "" : "w-full md:max-w-[300px]"} shadow-xl shadow-slate-100 z-10 bg-white md:flex relative`);
  }, m(R, nt) {
    O(R, t, nt), M(t, e), X.m(e, null), M(t, n), M(t, s), ct.m(s, null), M(t, o), M(t, r), M(r, a), M(a, l), M(a, c), M(r, h), M(r, f), M(t, d), M(t, u), Tt(p, u, null), M(u, m), U && U.m(u, null), M(t, x), M(t, _), lt.m(_, null), M(t, k), M(t, v), M(v, y), ht.m(y, null), P = true, H || (I = [(e.addEventListener("click", i[5], n), () => e.removeEventListener("click", i[5], n)), (f.addEventListener("click", i[6], n), () => f.removeEventListener("click", i[6], n))], H = true);
  }, p(R, [nt]) {
    K !== (K = B(R)) && (X.d(1), X = K(R), X && (X.c(), X.m(e, null))), j !== (j = E(R)) && (ct.d(1), ct = j(R), ct && (ct.c(), ct.m(s, null)));
    const Fe = {};
    nt & 1 && (Fe.active = R[0] === "overview"), nt & 8 && (Fe.className = `rounded-lg ${R[3] ? "md:justify-center" : ""}`), nt & 1032 && (Fe.$$scope = {dirty: nt, ctx: R}), p.$set(Fe), R[2] ? U ? (U.p(R, nt), nt & 4 && et(U, 1)) : (U = Hn(R), U.c(), et(U, 1), U.m(u, null)) : U && (ii(), rt(U, 1, 1, () => {
      U = null;
    }), ni()), (!P || nt & 8 && b !== (b = `${R[3] ? "hidden md:flex" : ""} flex flex-col flex-grow shrink-0 overflow-y-auto gap-4 mx-5`)) && g(u, "class", b), At !== (At = Rt(R)) && (lt.d(1), lt = At(R), lt && (lt.c(), lt.m(_, null))), (!P || nt & 8 && w !== (w = `${R[3] ? "hidden md:flex" : ""} flex flex-col justify-end gap-4 m-5 mt-10`)) && g(_, "class", w), vt === (vt = Bt(R)) && ht ? ht.p(R, nt) : (ht.d(1), ht = vt(R), ht && (ht.c(), ht.m(y, null))), (!P || nt & 8 && A !== (A = `${R[3] ? "hidden md:flex" : ""} flex flex-row justify-center gap-4 mb-5 mx-5`)) && g(v, "class", A), (!P || nt & 8 && C !== (C = `flex flex-col ${R[3] ? "" : "w-full md:max-w-[300px]"} shadow-xl shadow-slate-100 z-10 bg-white md:flex relative`)) && g(t, "class", C);
  }, i(R) {
    P || (et(p.$$.fragment, R), et(U), P = true);
  }, o(R) {
    rt(p.$$.fragment, R), rt(U), P = false;
  }, d(R) {
    R && D(t), X.d(), ct.d(), Et(p), U && U.d(), lt.d(), ht.d(), H = false, ee(I);
  }};
}
function Gr(i, t, e) {
  const n = fn();
  let s = false, {currentTab: o} = t, {connected: r} = t, {hasStats: a} = t;
  const l = u => {
    n("change", u);
  };
  hn(() => {
    const u = localStorage.getItem("dash_collapsed");
    u && e(3, s = u === "true");
  });
  const c = () => {
    e(3, s = !s), localStorage.setItem("dash_collapsed", s ? "true" : "false");
  }, h = () => {
    e(3, s = !s), localStorage.setItem("dash_collapsed", s ? "true" : "false");
  }, f = () => l("overview"), d = () => l("statistics");
  return i.$$set = u => {
    "currentTab" in u && e(0, o = u.currentTab), "connected" in u && e(1, r = u.connected), "hasStats" in u && e(2, a = u.hasStats);
  }, [o, r, a, s, l, c, h, f, d];
}
class Kr extends Nt {
  constructor(t) {
    super(), Wt(this, t, Gr, Qr, Ht, {currentTab: 0, connected: 1, hasStats: 2});
  }
}
function jn(i) {
  let t;
  return {c() {
    t = document.createElement("div"), g(t, "class", "w-2 h-2 bg-blue-300 rounded-full absolute top-2.5 right-2.5");
  }, m(e, n) {
    O(e, t, n);
  }, d(e) {
    e && D(t);
  }};
}
function Wn(i) {
  let t;
  return {c() {
    t = document.createElement("div"), g(t, "class", "w-2 h-2 bg-yellow-500 rounded-full absolute top-2.5 right-2.5 waiting-animation");
  }, m(e, n) {
    O(e, t, n);
  }, d(e) {
    e && D(t);
  }};
}
function Xr(i) {
  let t, e, n, s;
  return {c() {
    t = document.createElementNS("http://www.w3.org/2000/svg", "svg"), e = document.createElementNS("http://www.w3.org/2000/svg", "g"), n = document.createElementNS("http://www.w3.org/2000/svg", "rect"), s = document.createElementNS("http://www.w3.org/2000/svg", "path"), g(n, "id", "bound"), g(n, "x", "0"), g(n, "y", "0"), g(n, "width", "24"), g(n, "height", "24"), g(s, "d", "M15,7 L15,8 C15,8.55228475 15.4477153,9 16,9 C16.5522847,9 17,8.55228475 17,8 L17,7 L19,7 L19,10 C19,10.5522847 19.4477153,11 20,11 C20.5522847,11 21,10.5522847 21,10 L21,7 C22.1045695,7 23,7.8954305 23,9 L23,15 C23,16.1045695 22.1045695,17 21,17 L3,17 C1.8954305,17 1,16.1045695 1,15 L1,9 C1,7.8954305 1.8954305,7 3,7 L3,10 C3,10.5522847 3.44771525,11 4,11 C4.55228475,11 5,10.5522847 5,10 L5,7 L7,7 L7,8 C7,8.55228475 7.44771525,9 8,9 C8.55228475,9 9,8.55228475 9,8 L9,7 L11,7 L11,10 C11,10.5522847 11.4477153,11 12,11 C12.5522847,11 13,10.5522847 13,10 L13,7 L15,7 Z"), g(s, "id", "Combined-Shape"), g(s, "fill", "currentColor"), g(s, "transform", "translate(12.000000, 12.000000) rotate(-45.000000) translate(-12.000000, -12.000000) "), g(e, "id", "Stockholm-icons-/-Home-/-Ruller"), g(e, "stroke", "none"), g(e, "stroke-width", "1"), g(e, "fill", "none"), g(e, "fill-rule", "evenodd"), g(t, "width", "24px"), g(t, "height", "24px"), g(t, "viewBox", "0 0 24 24"), g(t, "version", "1.1"), g(t, "xmlns", "http://www.w3.org/2000/svg");
  }, m(o, r) {
    O(o, t, r), M(t, e), M(e, n), M(e, s);
  }, p: gt, d(o) {
    o && D(t);
  }};
}
function Jr(i) {
  let t;
  function e(o, r) {
    return o[0].value == 1 ? na : ia;
  }
  let n = e(i), s = n(i);
  return {c() {
    s.c(), t = document.createTextNode("");
  }, m(o, r) {
    s.m(o, r), O(o, t, r);
  }, p(o, r) {
    n !== (n = e(o)) && (s.d(1), s = n(o), s && (s.c(), s.m(t.parentNode, t)));
  }, d(o) {
    s.d(o), o && D(t);
  }};
}
function qr(i) {
  let t, e, n, s, o;
  return {c() {
    t = document.createElementNS("http://www.w3.org/2000/svg", "svg"), e = document.createElementNS("http://www.w3.org/2000/svg", "g"), n = document.createElementNS("http://www.w3.org/2000/svg", "rect"), s = document.createElementNS("http://www.w3.org/2000/svg", "path"), o = document.createElementNS("http://www.w3.org/2000/svg", "path"), g(n, "id", "Rectangle-5"), g(n, "x", "0"), g(n, "y", "0"), g(n, "width", "24"), g(n, "height", "24"), g(s, "d", "M20.5,11 L22.5,11 C23.3284271,11 24,11.6715729 24,12.5 C24,13.3284271 23.3284271,14 22.5,14 L20.5,14 C19.6715729,14 19,13.3284271 19,12.5 C19,11.6715729 19.6715729,11 20.5,11 Z M1.5,11 L3.5,11 C4.32842712,11 5,11.6715729 5,12.5 C5,13.3284271 4.32842712,14 3.5,14 L1.5,14 C0.671572875,14 1.01453063e-16,13.3284271 0,12.5 C-1.01453063e-16,11.6715729 0.671572875,11 1.5,11 Z"), g(s, "id", "Combined-Shape"), g(s, "fill", "currentColor"), g(s, "opacity", "0.3"), g(o, "d", "M12,16 C13.6568542,16 15,14.6568542 15,13 C15,11.3431458 13.6568542,10 12,10 C10.3431458,10 9,11.3431458 9,13 C9,14.6568542 10.3431458,16 12,16 Z M12,18 C9.23857625,18 7,15.7614237 7,13 C7,10.2385763 9.23857625,8 12,8 C14.7614237,8 17,10.2385763 17,13 C17,15.7614237 14.7614237,18 12,18 Z"), g(o, "id", "Oval-15"), g(o, "fill", "currentColor"), g(e, "id", "Stockholm-icons-/-Code-/-Commit"), g(e, "stroke", "none"), g(e, "stroke-width", "1"), g(e, "fill", "none"), g(e, "fill-rule", "evenodd"), g(t, "width", 24), g(t, "height", 24), g(t, "viewBox", "0 0 24 24"), g(t, "version", "1.1"), g(t, "xmlns", "http://www.w3.org/2000/svg"), g(t, "xmlns:xlink", "http://www.w3.org/1999/xlink");
  }, m(r, a) {
    O(r, t, a), M(t, e), M(e, n), M(e, s), M(e, o);
  }, p: gt, d(r) {
    r && D(t);
  }};
}
function Zr(i) {
  let t, e, n, s, o;
  return {c() {
    t = document.createElementNS("http://www.w3.org/2000/svg", "svg"), e = document.createElementNS("http://www.w3.org/2000/svg", "g"), n = document.createElementNS("http://www.w3.org/2000/svg", "path"), s = document.createElementNS("http://www.w3.org/2000/svg", "rect"), o = document.createElementNS("http://www.w3.org/2000/svg", "path"), g(n, "d", "M0 0h24v24H0z"), g(s, "fill", "currentColor"), g(s, "x", "4"), g(s, "y", "5"), g(s, "width", "16"), g(s, "height", "3"), g(s, "rx", "1.5"), g(o, "d", "M5.5 15h13a1.5 1.5 0 010 3h-13a1.5 1.5 0 010-3zm0-5h7a1.5 1.5 0 010 3h-7a1.5 1.5 0 010-3z"), g(o, "fill", "currentColor"), g(o, "opacity", ".3"), g(e, "fill", "none"), g(e, "fill-rule", "evenodd"), g(t, "width", 24), g(t, "height", 24), g(t, "viewBox", "0 0 24 24"), g(t, "xmlns", "http://www.w3.org/2000/svg");
  }, m(r, a) {
    O(r, t, a), M(t, e), M(e, n), M(e, s), M(e, o);
  }, p: gt, d(r) {
    r && D(t);
  }};
}
function $r(i) {
  let t;
  function e(o, r) {
    return o[0].symbol === "s" ? aa : o[0].symbol === "d" ? ra : o[0].symbol === "w" ? oa : sa;
  }
  let n = e(i), s = n(i);
  return {c() {
    s.c(), t = document.createTextNode("");
  }, m(o, r) {
    s.m(o, r), O(o, t, r);
  }, p(o, r) {
    n !== (n = e(o)) && (s.d(1), s = n(o), s && (s.c(), s.m(t.parentNode, t)));
  }, d(o) {
    s.d(o), o && D(t);
  }};
}
function ta(i) {
  let t, e, n, s;
  return {c() {
    t = document.createElementNS("http://www.w3.org/2000/svg", "svg"), e = document.createElementNS("http://www.w3.org/2000/svg", "g"), n = document.createElementNS("http://www.w3.org/2000/svg", "rect"), s = document.createElementNS("http://www.w3.org/2000/svg", "path"), g(n, "id", "bound"), g(n, "x", "0"), g(n, "y", "0"), g(n, "width", "24"), g(n, "height", "24"), g(s, "opacity", "0.6"), g(s, "d", "M12,21 C15.8659932,21 19,17.8659932 19,14 C19,11.4226712 16.6666667,8.08933783 12,4 C7.33333333,8.08933783 5,11.4226712 5,14 C5,17.8659932 8.13400675,21 12,21 Z"), g(s, "id", "Oval-2"), g(s, "fill", "currentColor"), g(e, "id", "Stockholm-icons-/-Design-/-Color"), g(e, "stroke", "none"), g(e, "stroke-width", "1"), g(e, "fill", "none"), g(e, "fill-rule", "evenodd"), g(t, "width", 24), g(t, "height", 24), g(t, "viewBox", "0 0 24 24"), g(t, "version", "1.1"), g(t, "xmlns", "http://www.w3.org/2000/svg"), g(t, "xmlns:xlink", "http://www.w3.org/1999/xlink");
  }, m(o, r) {
    O(o, t, r), M(t, e), M(e, n), M(e, s);
  }, p: gt, d(o) {
    o && D(t);
  }};
}
function ea(i) {
  let t, e, n, s;
  return {c() {
    t = document.createElementNS("http://www.w3.org/2000/svg", "svg"), e = document.createElementNS("http://www.w3.org/2000/svg", "g"), n = document.createElementNS("http://www.w3.org/2000/svg", "polygon"), s = document.createElementNS("http://www.w3.org/2000/svg", "path"), g(n, "id", "Shape"), g(n, "points", "0 0 24 0 24 24 0 24"), g(s, "d", "M18,16 C18,19.3137085 15.3137085,22 12,22 C8.6862915,22 6,19.3137085 6,16 C6,13.7791529 7.20659589,11.8401214 9,10.8026932 L9,5 C9,3.34314575 10.3431458,2 12,2 C13.6568542,2 15,3.34314575 15,5 L15,10.8026932 C16.7934041,11.8401214 18,13.7791529 18,16 Z M12,4 C11.4477153,4 11,4.44771525 11,5 L11,10 C11,10.5522847 11.4477153,11 12,11 C12.5522847,11 13,10.5522847 13,10 L13,5 C13,4.44771525 12.5522847,4 12,4 Z"), g(s, "id", "Combined-Shape"), g(s, "fill", "currentColor"), g(e, "id", "Stockholm-icons-/-Weather-/-Temperature-half"), g(e, "stroke", "none"), g(e, "stroke-width", "1"), g(e, "fill", "none"), g(e, "fill-rule", "evenodd"), g(t, "width", 24), g(t, "height", 24), g(t, "viewBox", "0 0 24 24"), g(t, "version", "1.1"), g(t, "xmlns", "http://www.w3.org/2000/svg");
  }, m(o, r) {
    O(o, t, r), M(t, e), M(e, n), M(e, s);
  }, p: gt, d(o) {
    o && D(t);
  }};
}
function ia(i) {
  let t, e, n, s;
  return {c() {
    t = document.createElementNS("http://www.w3.org/2000/svg", "svg"), e = document.createElementNS("http://www.w3.org/2000/svg", "g"), n = document.createElementNS("http://www.w3.org/2000/svg", "path"), s = document.createElementNS("http://www.w3.org/2000/svg", "path"), g(n, "d", "M0 0h24v24H0z"), g(s, "d", "M12.036 10.621l2.828-2.828a1 1 0 011.414 1.414l-2.828 2.829 2.828 2.828a1 1 0 01-1.414 1.414l-2.828-2.828-2.829 2.828a1 1 0 11-1.414-1.414l2.828-2.828-2.828-2.829a1 1 0 011.414-1.414l2.829 2.828z"), g(s, "fill", "currentColor"), g(e, "fill", "none"), g(e, "fill-rule", "evenodd"), g(t, "width", 28), g(t, "height", 28), g(t, "viewBox", "0 0 24 24"), g(t, "xmlns", "http://www.w3.org/2000/svg");
  }, m(o, r) {
    O(o, t, r), M(t, e), M(e, n), M(e, s);
  }, d(o) {
    o && D(t);
  }};
}
function na(i) {
  let t, e, n, s;
  return {c() {
    t = document.createElementNS("http://www.w3.org/2000/svg", "svg"), e = document.createElementNS("http://www.w3.org/2000/svg", "g"), n = document.createElementNS("http://www.w3.org/2000/svg", "path"), s = document.createElementNS("http://www.w3.org/2000/svg", "path"), g(n, "d", "M0 0h24v24H0z"), g(s, "d", "M16.769 7.818a1 1 0 011.462 1.364l-7 7.5a1 1 0 01-1.382.077l-3.5-3a1 1 0 011.302-1.518l2.772 2.376 6.346-6.8z"), g(s, "fill", "currentColor"), g(e, "fill", "none"), g(e, "fill-rule", "evenodd"), g(t, "width", 28), g(t, "height", 28), g(t, "viewBox", "0 0 24 24"), g(t, "xmlns", "http://www.w3.org/2000/svg");
  }, m(o, r) {
    O(o, t, r), M(t, e), M(e, n), M(e, s);
  }, d(o) {
    o && D(t);
  }};
}
function sa(i) {
  let t, e, n, s, o, r;
  return {c() {
    t = document.createElementNS("http://www.w3.org/2000/svg", "svg"), e = document.createElementNS("http://www.w3.org/2000/svg", "g"), n = document.createElementNS("http://www.w3.org/2000/svg", "path"), s = document.createElementNS("http://www.w3.org/2000/svg", "circle"), o = document.createElementNS("http://www.w3.org/2000/svg", "rect"), r = document.createElementNS("http://www.w3.org/2000/svg", "rect"), g(n, "d", "M0 0h24v24H0z"), g(s, "fill", "currentColor"), g(s, "opacity", ".3"), g(s, "cx", "12"), g(s, "cy", "12"), g(s, "r", "10"), g(o, "fill", "currentColor"), g(o, "x", "11"), g(o, "y", "10"), g(o, "width", "2"), g(o, "height", "7"), g(o, "rx", "1"), g(r, "fill", "currentColor"), g(r, "x", "11"), g(r, "y", "7"), g(r, "width", "2"), g(r, "height", "2"), g(r, "rx", "1"), g(e, "fill", "none"), g(e, "fill-rule", "evenodd"), g(t, "width", 28), g(t, "height", 28), g(t, "viewBox", "0 0 24 24"), g(t, "xmlns", "http://www.w3.org/2000/svg");
  }, m(a, l) {
    O(a, t, l), M(t, e), M(e, n), M(e, s), M(e, o), M(e, r);
  }, d(a) {
    a && D(t);
  }};
}
function oa(i) {
  let t, e, n, s, o, r;
  return {c() {
    t = document.createElementNS("http://www.w3.org/2000/svg", "svg"), e = document.createElementNS("http://www.w3.org/2000/svg", "g"), n = document.createElementNS("http://www.w3.org/2000/svg", "path"), s = document.createElementNS("http://www.w3.org/2000/svg", "circle"), o = document.createElementNS("http://www.w3.org/2000/svg", "rect"), r = document.createElementNS("http://www.w3.org/2000/svg", "rect"), g(n, "d", "M0 0h24v24H0z"), g(s, "fill", "currentColor"), g(s, "opacity", ".3"), g(s, "cx", "12"), g(s, "cy", "12"), g(s, "r", "10"), g(o, "fill", "currentColor"), g(o, "x", "11"), g(o, "y", "7"), g(o, "width", "2"), g(o, "height", "8"), g(o, "rx", "1"), g(r, "fill", "currentColor"), g(r, "x", "11"), g(r, "y", "16"), g(r, "width", "2"), g(r, "height", "2"), g(r, "rx", "1"), g(e, "fill", "none"), g(e, "fill-rule", "evenodd"), g(t, "width", 28), g(t, "height", 28), g(t, "viewBox", "0 0 24 24"), g(t, "xmlns", "http://www.w3.org/2000/svg");
  }, m(a, l) {
    O(a, t, l), M(t, e), M(e, n), M(e, s), M(e, o), M(e, r);
  }, d(a) {
    a && D(t);
  }};
}
function ra(i) {
  let t, e, n, s, o;
  return {c() {
    t = document.createElementNS("http://www.w3.org/2000/svg", "svg"), e = document.createElementNS("http://www.w3.org/2000/svg", "g"), n = document.createElementNS("http://www.w3.org/2000/svg", "path"), s = document.createElementNS("http://www.w3.org/2000/svg", "circle"), o = document.createElementNS("http://www.w3.org/2000/svg", "path"), g(n, "d", "M0 0h24v24H0z"), g(s, "fill", "currentColor"), g(s, "opacity", ".3"), g(s, "cx", "12"), g(s, "cy", "12"), g(s, "r", "10"), g(o, "d", "M12.036 10.621l2.828-2.828a1 1 0 011.414 1.414l-2.828 2.829 2.828 2.828a1 1 0 01-1.414 1.414l-2.828-2.828-2.829 2.828a1 1 0 11-1.414-1.414l2.828-2.828-2.828-2.829a1 1 0 011.414-1.414l2.829 2.828z"), g(o, "fill", "currentColor"), g(e, "fill", "none"), g(e, "fill-rule", "evenodd"), g(t, "width", 28), g(t, "height", 28), g(t, "viewBox", "0 0 24 24"), g(t, "xmlns", "http://www.w3.org/2000/svg");
  }, m(r, a) {
    O(r, t, a), M(t, e), M(e, n), M(e, s), M(e, o);
  }, d(r) {
    r && D(t);
  }};
}
function aa(i) {
  let t, e, n, s, o;
  return {c() {
    t = document.createElementNS("http://www.w3.org/2000/svg", "svg"), e = document.createElementNS("http://www.w3.org/2000/svg", "g"), n = document.createElementNS("http://www.w3.org/2000/svg", "path"), s = document.createElementNS("http://www.w3.org/2000/svg", "circle"), o = document.createElementNS("http://www.w3.org/2000/svg", "path"), g(n, "d", "M0 0h24v24H0z"), g(s, "fill", "currentColor"), g(s, "opacity", ".3"), g(s, "cx", "12"), g(s, "cy", "12"), g(s, "r", "10"), g(o, "d", "M16.769 7.818a1 1 0 011.462 1.364l-7 7.5a1 1 0 01-1.382.077l-3.5-3a1 1 0 011.302-1.518l2.772 2.376 6.346-6.8z"), g(o, "fill", "currentColor"), g(e, "fill", "none"), g(e, "fill-rule", "evenodd"), g(t, "width", 28), g(t, "height", 28), g(t, "viewBox", "0 0 24 24"), g(t, "xmlns", "http://www.w3.org/2000/svg");
  }, m(r, a) {
    O(r, t, a), M(t, e), M(e, n), M(e, s), M(e, o);
  }, d(r) {
    r && D(t);
  }};
}
function la(i) {
  let t, e = i[0].name + "", n, s, o, r, a, l = !["status"].includes(i[0].type) && i[0].symbol, c = l && Nn(i);
  return {c() {
    t = document.createElement("span"), n = document.createTextNode(e), s = document.createTextNode(" "), o = document.createElement("span"), r = document.createTextNode(i[1]), a = document.createTextNode(" "), c && c.c(), g(t, "class", "text-gray-400 font-normal"), g(o, "class", "font-semibold text-lg");
  }, m(h, f) {
    O(h, t, f), M(t, n), O(h, s, f), O(h, o, f), M(o, r), M(o, a), c && c.m(o, null);
  }, p(h, f) {
    f & 1 && e !== (e = h[0].name + "") && ut(n, e), f & 2 && ut(r, h[1]), f & 1 && (l = !["status"].includes(h[0].type) && h[0].symbol), l ? c ? c.p(h, f) : (c = Nn(h), c.c(), c.m(o, null)) : c && (c.d(1), c = null);
  }, d(h) {
    h && D(t), h && D(s), h && D(o), c && c.d();
  }};
}
function ca(i) {
  let t, e = i[0].name + "", n;
  return {c() {
    t = document.createElement("span"), n = document.createTextNode(e), g(t, "class", "text-gray-400 font-normal");
  }, m(s, o) {
    O(s, t, o), M(t, n);
  }, p(s, o) {
    o & 1 && e !== (e = s[0].name + "") && ut(n, e);
  }, d(s) {
    s && D(t);
  }};
}
function ha(i) {
  let t, e = i[0].name + "", n, s, o, r, a, l = (i[0].symbol ? i[0].symbol : "") + "", c, h, f, d, u, p, m, b, x;
  return {c() {
    t = document.createElement("span"), n = document.createTextNode(e), s = document.createTextNode(` -
                `), o = document.createElement("span"), r = document.createTextNode(i[1]), a = document.createElement("span"), c = document.createTextNode(l), h = document.createTextNode(" "), f = document.createElement("div"), d = document.createElement("input"), g(a, "class", "font-normal"), g(t, "class", "text-gray-400 font-normal"), g(d, "class", "slider w-full mt-2"), g(d, "step", u = i[0].step), g(d, "min", p = i[0].min), g(d, "max", m = i[0].max), g(d, "type", "range");
  }, m(_, w) {
    O(_, t, w), M(t, n), M(t, s), M(t, o), M(o, r), M(o, a), M(a, c), O(_, h, w), O(_, f, w), M(f, d), In(d, i[1]), b || (x = [(d.addEventListener("change", i[6], n), () => d.removeEventListener("change", i[6], n)), (d.addEventListener("input", i[6], n), () => d.removeEventListener("input", i[6], n))], b = true);
  }, p(_, w) {
    w & 1 && e !== (e = _[0].name + "") && ut(n, e), w & 2 && ut(r, _[1]), w & 1 && l !== (l = (_[0].symbol ? _[0].symbol : "") + "") && ut(c, l), w & 1 && u !== (u = _[0].step) && g(d, "step", u), w & 1 && p !== (p = _[0].min) && g(d, "min", p), w & 1 && m !== (m = _[0].max) && g(d, "max", m), w & 2 && In(d, _[1]);
  }, d(_) {
    _ && D(t), _ && D(h), _ && D(f), b = false, ee(x);
  }};
}
function fa(i) {
  let t, e = i[0].name + "", n, s, o, r = i[4](i[1], i[0].min, i[0].max) + "", a, l, c = i[0].symbol + "", h, f, d, u, p;
  return {c() {
    t = document.createElement("span"), n = document.createTextNode(e), s = document.createTextNode(` -
                `), o = document.createElement("span"), a = document.createTextNode(r), l = document.createElement("span"), h = document.createTextNode(c), f = document.createTextNode(" "), d = document.createElement("div"), u = document.createElement("div"), g(l, "class", "font-normal"), g(t, "class", "text-gray-400 font-normal"), g(u, "class", "bg-blue-600 h-2.5 rounded-full transition-all"), g(u, "style", p = `width: ${i[4](i[0].value, i[0].min, i[0].max)}%`), g(d, "class", "w-full bg-gray-200 rounded-full h-2.5 mt-2");
  }, m(m, b) {
    O(m, t, b), M(t, n), M(t, s), M(t, o), M(o, a), M(o, l), M(l, h), O(m, f, b), O(m, d, b), M(d, u);
  }, p(m, b) {
    b & 1 && e !== (e = m[0].name + "") && ut(n, e), b & 3 && r !== (r = m[4](m[1], m[0].min, m[0].max) + "") && ut(a, r), b & 1 && c !== (c = m[0].symbol + "") && ut(h, c), b & 1 && p !== (p = `width: ${m[4](m[0].value, m[0].min, m[0].max)}%`) && g(u, "style", p);
  }, d(m) {
    m && D(t), m && D(f), m && D(d);
  }};
}
function Nn(i) {
  let t, e = i[0].symbol + "", n;
  return {c() {
    t = document.createElement("small"), n = document.createTextNode(e), g(t, "class", "font-normal text-gray-500");
  }, m(s, o) {
    O(s, t, o), M(t, n);
  }, p(s, o) {
    o & 1 && e !== (e = s[0].symbol + "") && ut(n, e);
  }, d(s) {
    s && D(t);
  }};
}
function da(i) {
  let t, e, n, s, o, r, a, l, c, h = i[2] && jn(), f = i[0].value != i[1] && Wn();
  function d(_, w) {
    return _[0].type === "temperature" ? ea : _[0].type === "humidity" ? ta : _[0].type === "status" ? $r : _[0].type === "progress" ? Zr : _[0].type === "slider" ? qr : _[0].type === "button" ? Jr : Xr;
  }
  let u = d(i), p = u(i);
  function m(_, w) {
    return _[0].type === "progress" ? fa : _[0].type === "slider" ? ha : _[0].type === "button" ? ca : la;
  }
  let b = m(i), x = b(i);
  return {c() {
    t = document.createElement("div"), h && h.c(), e = document.createTextNode(" "), f && f.c(), n = document.createTextNode(" "), s = document.createElement("div"), p.c(), r = document.createTextNode(" "), a = document.createElement("div"), x.c(), g(s, "class", o = `flex flex-col items-center justify-center p-4 rounded-xl ${i[3]} ${i[0].type === "button" || i[0].type === "text" ? "order-last cursor-pointer" : ""}`), g(a, "class", "flex flex-col grow justify-start gap-0.5"), g(t, "class", "col-span-12 md:col-span-6 lg:col-span-4 xl:col-span-3 2xl:col-span-2 row-span-1 h-[100px] p-5 flex flex-row items-center gap-4 bg-white border border-gray-100 rounded-xl relative");
  }, m(_, w) {
    O(_, t, w), h && h.m(t, null), M(t, e), f && f.m(t, null), M(t, n), M(t, s), p.m(s, null), M(t, r), M(t, a), x.m(a, null), l || (c = (s.addEventListener("click", i[5], n), () => s.removeEventListener("click", i[5], n)), l = true);
  }, p(_, [w]) {
    _[2] ? h || (h = jn(), h.c(), h.m(t, e)) : h && (h.d(1), h = null), _[0].value != _[1] ? f || (f = Wn(), f.c(), f.m(t, n)) : f && (f.d(1), f = null), u === (u = d(_)) && p ? p.p(_, w) : (p.d(1), p = u(_), p && (p.c(), p.m(s, null))), w & 9 && o !== (o = `flex flex-col items-center justify-center p-4 rounded-xl ${_[3]} ${_[0].type === "button" || _[0].type === "text" ? "order-last cursor-pointer" : ""}`) && g(s, "class", o), b === (b = m(_)) && x ? x.p(_, w) : (x.d(1), x = b(_), x && (x.c(), x.m(a, null)));
  }, i: gt, o: gt, d(_) {
    _ && D(t), h && h.d(), f && f.d(), p.d(), x.d(), l = false, c();
  }};
}
function ua(i, t, e) {
  const n = fn();
  let {data: s = {}} = t, o = false, r = 0, a = false, l = "", c = null;
  const h = m => {
    !o || c !== null || r !== m && (e(1, r = m), e(2, a = true), setTimeout(() => {
      e(2, a = false);
    }, 100));
  }, f = m => {
    !o || s.value === m || (s.type === "slider" ? (c && clearTimeout(c), c = setTimeout(() => {
      n("update", {command: "slider:changed", id: s.id, value: m}), c = null;
    }, 300)) : s.type === "button" && n("update", {command: "button:clicked", id: s.id, value: m}));
  }, d = (m, b, x) => {
    let _ = (m - b) / (x - b) * 100;
    return _ < 0 && (_ = 0), Math.round(_ * 100) / 100;
  };
  hn(() => {
    e(1, r = s.value), o = true;
  });
  const u = () => s.type === "button" ? e(1, r = !s.value) : null;
  function p() {
    r = kr(this.value), e(1, r);
  }
  return i.$$set = m => {
    "data" in m && e(0, s = m.data);
  }, i.$$.update = () => {
    if (i.$$.dirty & 2 && f(r), i.$$.dirty & 1 && h(s.value), i.$$.dirty & 1) switch (s.type) {
      case "temperature":
        e(3, l = "bg-rose-100 bg-opacity-50 text-rose-500");
        break;
      case "humidity":
        e(3, l = "bg-blue-100 bg-opacity-50 text-blue-500");
        break;
      case "status":
        switch (s.symbol) {
          case "s":
            e(3, l = "bg-green-100 bg-opacity-50 text-green-500");
            break;
          case "d":
            e(3, l = "bg-red-100 bg-opacity-50 text-red-500");
            break;
          case "w":
            e(3, l = "bg-yellow-100 bg-opacity-50 text-yellow-500");
            break;
          default:
            e(3, l = "bg-gray-200 bg-opacity-50 text-gray-500");
            break;
        }
        break;
      case "progress":
        e(3, l = "bg-blue-100 bg-opacity-50 text-blue-500");
        break;
      case "slider":
        e(3, l = "bg-blue-100 bg-opacity-50 text-blue-500");
        break;
      case "text":
        e(3, l = "bg-green-500 bg-opacity-90 hover:bg-green-600 transition-colors text-white");
        break;
      case "button":
        s.value === 1 ? e(3, l = "bg-green-500 bg-opacity-90 hover:bg-green-600 transition-colors text-white") : e(3, l = "bg-gray-500 bg-opacity-90 hover:bg-gray-600 transition-colors text-white");
        break;
      default:
        e(3, l = "bg-gray-200 bg-opacity-50 text-gray-500");
    }
  }, [s, r, a, l, d, u, p];
}
class ga extends Nt {
  constructor(t) {
    super(), Wt(this, t, ua, da, Ht, {data: 0});
  }
}
function pa(i, t) {
  let e = ["children", "$$scope", "$$slots"].concat(t);
  const n = {};
  for (const s of Object.keys(i)) e.includes(s) || (n[s] = i[s]);
  return n;
}
