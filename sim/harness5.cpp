// harness5: runs the real sender firmware on a synthetic person (everyone model).
// Extra person params: TRA TRF (head/eye tremor on V), TRE (tremor EMG), NYS NYF (vertical nystagmus),
//   MAINS (50 Hz on both), BT (blink time scale), SPE SPD (spasm EMG / DC), MA (head movement), YQE YHE (yawn)
// Extra events: X<d>@t spasm | M<d>@t head movement | Y<d>@t yawn | H<d>@t held bite | P<pin>:<d>@t button
// env: MODE=blink|clench  N=events  BA=blink amp  BR=blink return ratio  BE=blink EMG amp (raw)
//      BW=blink EMG burst width (s)  JIT=amplitude jitter fraction  CE=clench EMG amp on temples (raw)
//      CV=fraction of clench EMG also on forehead  SEED=rng seed  TS / BS = thrScale / biteScale at boot
#include "Arduino.h"
#include "WiFi.h"
#include "esp_now.h"
#include "Preferences.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>
#include <vector>
static uint64_t g_us = 0;
uint32_t micros(){ g_us += 17; return (uint32_t)g_us; }
uint32_t millis(){ return (uint32_t)(g_us/1000); }
void delay(uint32_t ms){ g_us += (uint64_t)ms*1000; }
HS Serial; WiFiC WiFi;
struct Key{ double t; char c; bool sent; }; static std::vector<Key> keys;
int HS::available(){ for(auto&k:keys) if(!k.sent && g_us/1e6>=k.t) return 1; return 0; }
int HS::read(){ for(auto&k:keys) if(!k.sent && g_us/1e6>=k.t){ k.sent=true; return k.c; } return -1; }
static bool lineStart=true;
size_t HS::write(const uint8_t* d,size_t n){
  static int ts=getenv("TSLOG")?1:0;
  if(!ts){ fwrite(d,1,n,stdout); return n; }
  for(size_t i=0;i<n;i++){ if(lineStart){ printf("%9.3f| ",g_us/1e6); lineStart=false; } putchar(d[i]); if(d[i]=='\n') lineStart=true; }
  return n; }
void digitalWrite(uint8_t,uint8_t){}
struct Btn{ double t,d; int pin; }; static std::vector<Btn> btns;
int digitalRead(uint8_t pin){ double t=g_us/1e6; for(auto&b:btns) if((int)pin==b.pin && t>=b.t && t<b.t+b.d) return 0; return 1; }
static bool g_buzz=false; static FILE* bzLog=nullptr;
double ledcSetup(uint8_t, double f, uint8_t){ return f; }
void ledcAttachPin(uint8_t, uint8_t){}
void ledcWrite(uint8_t, uint32_t duty){ bool on=duty>0; if(on!=g_buzz && bzLog) fprintf(bzLog,"%.3f %d\n",g_us/1e6,(int)on); g_buzz=on; }
static FILE* pktOut=nullptr; static char pktLast=0;
esp_err_t esp_now_send(const uint8_t*,const uint8_t* d,size_t n){
  if (pktOut && n==4) fprintf(pktOut,"%llu %c %u\n",(unsigned long long)g_us,(char)d[2],(unsigned)d[3]);
  (void)pktLast; return ESP_OK; }
static double env(const char* k,double d){ const char* v=getenv(k); return v?atof(v):d; }
static std::mt19937_64 gen((uint64_t)env("SEED",1));
static std::normal_distribution<double> G(0.0,1.0);
static double nV(){ return G(gen); } static double nH(){ return G(gen); }
struct Ev{ double t; int kind; double a, e; double fV[10], pV[10], fH[10], pH[10]; };
static double emgSig(const double* f,const double* p,double t){ double v=0; for(int k=0;k<10;k++) v+=sin(2*M_PI*f[k]*t+p[k]); return v/sqrt(5.0); }
static std::vector<Ev> evs;
static double BR, BW, CV, CDUR=1.0;
static double BT=1, TRA=0, TRF=5, TRE=0, NYS=0, NYF=3, MAINS=0, SPD=0, MA=120, YQE=60, YHE=80;
static double gTf[10], gTp[10];
static double vert(double t){
  double v = 2048 + env("VN",8)*nV() + 6*sin(2*M_PI*50*t) + env("VDR",0)*(sin(2*M_PI*0.17*t+0.7)+0.7*sin(2*M_PI*0.53*t+2.2)+0.5*sin(2*M_PI*1.1*t+0.1));
  for (auto&e:evs){ double x=t-e.t;
    if (e.kind==0 && x>-0.05 && x<0.6*BT){ const double xs=x/BT;
      v += e.a*exp(-pow((xs-0.08)/0.045,2)) - BR*e.a*exp(-pow((xs-0.22)/0.07,2));
      v += e.e*exp(-pow((xs-0.06)/BW,2))*emgSig(e.fV,e.pV,t); // eyelid-muscle burst (40-150 Hz)
    }
    if (e.kind==12 && x>0 && x<e.a){ double en=(x<0.05? x/0.05 : x>e.a-0.1? (e.a-x)/0.1 : 1.0);
      v += en*(CV*e.e*emgSig(e.fV,e.pV,t) + env("CDC",0.3)*e.e*std::min(1.0,0.7+0.3*exp(-(x)/1.5))); }
    if (e.kind==13 && x>0 && x<e.a){ double en=(x<0.04? x/0.04 : x>e.a-0.08? std::max(0.0,(e.a-x)/0.08) : 1.0);
      v += en*(0.6*e.e*emgSig(e.fV,e.pV,t) + SPD*(e.e/250.0)); }
    if (e.kind==10 && x>0 && x<e.a) v += 0.5*MA*sin(2*M_PI*1.1*x)*(x<0.3? x/0.3:1.0) + 30*sin(2*M_PI*0.4*x);
    if (e.kind==14 && x>0 && x<e.a){ double en=(x<0.6? x/0.6 : x>e.a-0.5? std::max(0.0,(e.a-x)/0.5) : 1.0);
      v += en*(YQE*emgSig(e.fV,e.pV,t) + 60.0); }
    if (e.kind==3 && x>0 && x<e.a+0.15){ double en=(x<0.12? x/0.12 : x>e.a? (e.a+0.15-x)/0.15 : 1.0);
      v += en*(e.e*emgSig(e.fV,e.pV,t) + env("SQDC",250)*(1.0-exp(-x/0.15))) + env("SQLOBE",0)*exp(-pow((x-0.08)/0.045,2)); }
    if (e.kind==4 && x>0 && x<e.a){ double ph=fmod(x,0.65); if(ph<0.30) v += env("CHV",0.6)*e.e*emgSig(e.fV,e.pV,t); }
    if (e.kind==5 && x>0 && x<e.a){ double ph=fmod(x*7.3+sin(x*3.1),1.0); if(ph<0.55) v += 0.5*e.e*emgSig(e.fV,e.pV,t); }
    if (e.kind==7 && x>0 && x<e.a){ double en=(x<0.1? x/0.1 : x>e.a-0.1? (e.a-x)/0.1 : 1.0); v += en*e.e*emgSig(e.fV,e.pV,t); }
    if (e.kind==6 && x>0 && x<e.a){ double w=fmod(x,1.2); if(w<0.85 && fmod(x*4.5,1.0)<0.5) v += 0.3*e.e*emgSig(e.fV,e.pV,t); }
    if (e.kind==2 && x>0 && x<1.6){ double up=1.0/(1.0+exp(-(x-0.015)/0.006)), dn=1.0/(1.0+exp(-(x-1.215)/0.006)); v += e.a*(up-dn); }
    if (e.kind==1 && x>0 && x<CDUR){
      double en=(x<0.05? x/0.05 : x>CDUR-0.1? (CDUR-x)/0.1 : 1.0);
      v += en*(CV*e.e*emgSig(e.fV,e.pV,t) + env("CDC",0.3)*e.e) + env("CTR",0.0)*e.e*(x/0.03<1? x/0.03:1.0)*exp(-x/env("CTT",0.06));  // EMG + DC shift + bite movement transient
    }
  }
  if (g_buzz) v += env("BZV",0);
  v += TRA*sin(2*M_PI*TRF*t) + NYS*sin(2*M_PI*NYF*t+1.0) + MAINS*sin(2*M_PI*50*t);
  if (TRE>0) v += TRE*std::max(0.0,sin(2*M_PI*TRF*t))*emgSig(gTf,gTp,t);
  return v<0?0:v>4095?4095:v;
}
static double horiz(double t){
  double v = 2048 + env("HN",8)*nH() + env("HDR",0)*(sin(2*M_PI*0.11*t+1.3)+0.6*sin(2*M_PI*0.37*t+0.4)+0.4*sin(2*M_PI*0.83*t+2.1));
  for (auto&e:evs){ double x=t-e.t;
    if (e.kind==1 && x>0 && x<CDUR){ double en=(x<0.05? x/0.05 : x>CDUR-0.1? (CDUR-x)/0.1 : 1.0); v += en*e.e*emgSig(e.fH,e.pH,t); } // temporalis
    if (e.kind==8 && x>0 && x<1.6){ double up=1.0/(1.0+exp(-(x-0.03)/0.012)), dn=1.0/(1.0+exp(-(x-1.23)/0.012)); v += e.a*(up-dn); }
    if (e.kind==3 && x>0 && x<e.a) v += env("SQH",0.25)*e.e*emgSig(e.fH,e.pH,t);
    if (e.kind==4 && x>0 && x<e.a){ double ph=fmod(x,0.65); if(ph<0.30) v += e.e*emgSig(e.fH,e.pH,t); }
    if (e.kind==5 && x>0 && x<e.a){ double ph=fmod(x*7.3+sin(x*3.1),1.0); if(ph<0.55) v += 0.4*e.e*emgSig(e.fH,e.pH,t); }
    if (e.kind==6 && x>0 && x<e.a){ double w=fmod(x,1.2); if(w<0.85 && fmod(x*4.5,1.0)<0.5) v += 0.5*e.e*emgSig(e.fH,e.pH,t); }
    if (e.kind==0 && x>-0.05 && x<0.6*BT) v += env("BH",0.15)*e.e*exp(-pow((x/BT-0.06)/BW,2))*emgSig(e.fH,e.pH,t);               // a little blink EMG at temples
    if (e.kind==12 && x>0 && x<e.a){ double en=(x<0.05? x/0.05 : x>e.a-0.1? (e.a-x)/0.1 : 1.0); v += en*e.e*emgSig(e.fH,e.pH,t); }
    if (e.kind==13 && x>0 && x<e.a){ double en=(x<0.04? x/0.04 : x>e.a-0.08? std::max(0.0,(e.a-x)/0.08) : 1.0); v += en*e.e*emgSig(e.fH,e.pH,t); }
    if (e.kind==10 && x>0 && x<e.a) v += 0.3*MA*sin(2*M_PI*1.1*x+0.5) + 0.25*MA*0.3*emgSig(e.fH,e.pH,t);
    if (e.kind==14 && x>0 && x<e.a){ double en=(x<0.6? x/0.6 : x>e.a-0.5? std::max(0.0,(e.a-x)/0.5) : 1.0); v += en*YHE*emgSig(e.fH,e.pH,t); }
  }
  if (g_buzz) v += env("BZH",0);
  v += MAINS*sin(2*M_PI*50*t+0.4) + 0.3*TRA*sin(2*M_PI*TRF*t+0.3);
  if (TRE>0) v += 1.3*TRE*std::max(0.0,sin(2*M_PI*TRF*t))*emgSig(gTf,gTp,t);
  return v<0?0:v>4095?4095:v;
}
int analogRead(uint8_t pin){ double t=g_us/1e6; return (int)lround(pin==1? vert(t) : horiz(t)); }
void setup(); void loop();
int main(){
  const char* mode=getenv("MODE"); bool clench = mode && !strcmp(mode,"clench");
  int N=(int)env("N",30); double BA=env("BA",300), BE=env("BE",150), JIT=env("JIT",0.15), CE=env("CE",200);
  CDUR=env("CD",1.0); BR=env("BR",0.75); BW=env("BW",0.04); CV=env("CV",0.3);
  BT=env("BT",1); TRA=env("TRA",0); TRF=env("TRF",5); TRE=env("TRE",0); NYS=env("NYS",0); NYF=env("NYF",3);
  MAINS=env("MAINS",0); SPD=env("SPD",80); MA=env("MA",120); YQE=env("YQE",60); YHE=env("YHE",80);
  { std::mt19937_64 tg((uint64_t)env("SEED",1)*77ULL+5); std::uniform_real_distribution<double> F(40.0,150.0), P(0.0,2*M_PI); for(int k=0;k<10;k++){ gTf[k]=F(tg); gTp[k]=P(tg);} }
  std::uniform_real_distribution<double> J(1.0-JIT,1.0+JIT);
  double tEnd=8.0+N*2.6+2.0;
  if (getenv("SCHED")) {                           // "B@t" blink, "C@t" clench, "K<c>@t" serial key
    std::string sc=getenv("SCHED"); size_t p=0; tEnd=0;
    while (p<sc.size()){ size_t q=sc.find(',',p); if(q==std::string::npos) q=sc.size(); std::string it=sc.substr(p,q-p); p=q+1;
      size_t at=it.find('@'); double t=atof(it.substr(at+1).c_str()); if(t+3>tEnd) tEnd=t+3;
      std::mt19937_64 gen((uint64_t)env("SEED",1)*1000003ULL + (uint64_t)llround(t*1000.0)*7919ULL + (uint64_t)it[0]);  // per-event: adding later events never changes earlier ones
      if (it[0]=='b') evs.push_back(Ev{t,0,env("WEAK",0.65)*BA*J(gen),env("WEAK",0.65)*BE*J(gen),{},{},{},{}});
      else if (it[0]=='B') evs.push_back(Ev{t,0,BA*J(gen),BE*J(gen),{},{},{},{}});
      else if (it[0]=='C') evs.push_back(Ev{t,1,0,CE*J(gen),{},{},{},{}});
      else if (it[0]=='E') { double a=atof(it.substr(1,at-1).c_str()); evs.push_back(Ev{t,2,a,0,{},{},{},{}}); }
      else if (it[0]=='q') { double d=atof(it.substr(1,at-1).c_str()); evs.push_back(Ev{t,7,d,env("FQE",70)*J(gen),{},{},{},{}}); }
      else if (it[0]=='Q') { double d=atof(it.substr(1,at-1).c_str()); evs.push_back(Ev{t,3,d,env("SQE",300)*J(gen),{},{},{},{}}); }
      else if (it[0]=='W') { double d=atof(it.substr(1,at-1).c_str()); evs.push_back(Ev{t,4,d,env("CHE",250),{},{},{},{}}); }
      else if (it[0]=='T') { double d=atof(it.substr(1,at-1).c_str()); evs.push_back(Ev{t,5,d,env("TKE",250),{},{},{},{}}); }
      else if (it[0]=='S') { double d=atof(it.substr(1,at-1).c_str()); evs.push_back(Ev{t,6,d,env("TKE",250),{},{},{},{}}); }
      else if (it[0]=='h') { double a=atof(it.substr(1,at-1).c_str()); evs.push_back(Ev{t,8,a,0,{},{},{},{}}); }
      else if (it[0]=='H') { double d=atof(it.substr(1,at-1).c_str()); evs.push_back(Ev{t,12,d,env("CE",200)*J(gen),{},{},{},{}}); }
      else if (it[0]=='X') { double d=atof(it.substr(1,at-1).c_str()); evs.push_back(Ev{t,13,d,env("SPE",300)*J(gen),{},{},{},{}}); }
      else if (it[0]=='M') { double d=atof(it.substr(1,at-1).c_str()); evs.push_back(Ev{t,10,d,0,{},{},{},{}}); }
      else if (it[0]=='Y') { double d=atof(it.substr(1,at-1).c_str()); evs.push_back(Ev{t,14,d,0,{},{},{},{}}); }
      else if (it[0]=='P') { size_t c=it.find(':'); int pin=atoi(it.substr(1,c-1).c_str()); double d=atof(it.substr(c+1,at-c-1).c_str()); btns.push_back({t,d,pin}); }
      else if (it[0]=='K') keys.push_back({t,it[1],false}); }
  } else {
    for (int i=0;i<N;i++){ double t=8.0+i*2.6; if (clench) evs.push_back(Ev{t,1,0,CE*J(gen),{},{},{},{}}); else evs.push_back(Ev{t,0,BA*J(gen),BE*J(gen),{},{},{},{}}); }
  }
  { std::uniform_real_distribution<double> F(40.0,150.0), P(0.0,2*M_PI);
    for (auto&e:evs){ std::mt19937_64 eg((uint64_t)env("SEED",1)*2654435761ULL + (uint64_t)llround(e.t*1000.0)*104729ULL + (uint64_t)e.kind);
      for(int k=0;k<10;k++){ e.fV[k]=F(eg); e.pV[k]=P(eg); e.fH[k]=F(eg); e.pH[k]=P(eg); } } }
  if (getenv("PKT_OUT")) pktOut=fopen(getenv("PKT_OUT"),"w");
  if (getenv("BZ_OUT")) bzLog=fopen(getenv("BZ_OUT"),"w");
  if (getenv("NVS_IN")) { FILE* f=fopen(getenv("NVS_IN"),"rb"); uint32_t kl,vl; char kb[64];   // whole flash store
    while (f && fread(&kl,4,1,f)==1 && kl<64 && fread(kb,1,kl,f)==kl && fread(&vl,4,1,f)==1) {
      std::vector<uint8_t> v(vl); if (vl && fread(v.data(),1,vl,f)!=vl) break; Preferences::store()[std::string(kb,kl)]=v; }
    if (f) fclose(f); }
  if (getenv("PREF_IN")) { FILE* f=fopen(getenv("PREF_IN"),"rb"); if(f){ std::vector<uint8_t> b(4096); size_t n=fread(b.data(),1,b.size(),f); fclose(f); b.resize(n); Preferences::store()["train"]=b; } }
  setup();
  while (g_us < (uint64_t)(tEnd*1e6)) loop();
  if (getenv("NVS_OUT")) { FILE* f=fopen(getenv("NVS_OUT"),"wb");
    for (auto& kv: Preferences::store()) { uint32_t kl=kv.first.size(), vl=kv.second.size();
      fwrite(&kl,4,1,f); fwrite(kv.first.data(),1,kl,f); fwrite(&vl,4,1,f); fwrite(kv.second.data(),1,vl,f); }
    fclose(f); }
  if (getenv("PREF_OUT")) { FILE* f=fopen(getenv("PREF_OUT"),"wb"); auto it=Preferences::store().find("train"); if(f){ if(it!=Preferences::store().end()) fwrite(it->second.data(),1,it->second.size(),f); fclose(f);} }
}
