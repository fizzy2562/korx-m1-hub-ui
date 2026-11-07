import fs from 'fs';
const m=new WebAssembly.Module(fs.readFileSync(process.argv[2]));
console.log("imports",JSON.stringify(WebAssembly.Module.imports(m)));
console.log("exports",WebAssembly.Module.exports(m).map(e=>e.name).join(","));
