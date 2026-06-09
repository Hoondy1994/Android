const fs = require("fs");
const path = require("path");

function u(s) {
  return s.replace(/\\u([0-9a-fA-F]{4})/g, (_, h) =>
    String.fromCharCode(parseInt(h, 16))
  );
}

const py = fs.readFileSync(path.join(__dirname, "gen_svg.py"), "utf8");

function extract(name) {
  const re = new RegExp(`${name} = """([\\s\\S]*?)"""`);
  const m = py.match(re);
  if (!m) throw new Error(`${name} block not found in gen_svg.py`);
  return u(m[1]);
}

fs.writeFileSync(path.join(__dirname, "architecture.svg"), extract("ARCH"), "utf8");
fs.writeFileSync(path.join(__dirname, "flow.svg"), extract("FLOW"), "utf8");
console.log("Wrote architecture.svg and flow.svg");
