// Compare native movement to the actual recovered browser implementation.
const fs=require('fs'),vm=require('vm'),{spawnSync}=require('child_process');
const html=fs.readFileSync('reference/special-delivery.html','utf8');
const reference=html.slice(html.indexOf('function duck()'),html.indexOf('function proj('));
for(let wind=0;wind<3;wind++){
 const context=vm.createContext({});
 vm.runInContext(`const $=()=>({textContent:''});const k=new Set();let mode='play',p={x:0,z:2,y:0,v:0},cp=0,cam=2,time=0,weather=${wind},wt=10000,reward=0;const checkpoints=[2,20,32,48],gaps=[[13,16],[36,43],[49,52]];${reference}`,context);
 const expected=[],input=[];
 for(let i=0;i<2200;i++){
  const state=vm.runInContext('({p,mode})',context);if(state.mode!=='play')break;
  const p=state.p,side=p.x>.2?-1:p.x<-.2?1:0;
  const jump=p.y===0&&((p.z>=11.5&&p.z<13)||(p.z>=34.5&&p.z<36)||(p.z>=47.5&&p.z<49));
  const duck=p.z>25&&p.z<30,glide=(p.z>=11.5&&p.z<16.5)||(p.z>=34.5&&p.z<44)||(p.z>=47.5&&p.z<53);
  input.push([side,1,+jump,+duck,+glide].join(' '));
  vm.runInContext(`k.clear();k.add('w');${side<0?"k.add('a');":side>0?"k.add('d');":''}${duck?"k.add('shift');":''}${glide?"k.add(' ');":''}${jump?"p.y=.01;p.v=10;":''}tick(1/60);`,context);
  expected.push(vm.runInContext('[p.x,p.z,p.y,p.v,cam,cp,mode===\'reward\'?1:0]',context));
 }
 const run=spawnSync(process.argv[2],[String(wind)],{input:input.join('\n')+'\n',encoding:'utf8'});if(run.status!==0)throw Error(run.stderr);
 const actual=run.stdout.trim().split('\n').map(x=>x.split(' ').map(Number));
 if(actual.length!==expected.length)throw Error('Trace length differs');
 let max=0;for(let i=0;i<actual.length;i++)for(let j=0;j<7;j++){let err=Math.abs(actual[i][j]-expected[i][j]);max=Math.max(max,err);if(err>(j<5?.005:0))throw Error(`Wind ${wind} frame ${i} field ${j}: native ${actual[i][j]} reference ${expected[i][j]}`);}
 if(expected.at(-1)[6]!==1)throw Error('Reference route did not finish');
 console.log(`Wind ${wind}: ${actual.length} frames match original; max float error ${max.toFixed(5)}`);
}
