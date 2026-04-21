#include <iostream>
#include <list>
#include <functional>
#include <string>
#include <map>
using namespace std;
using Command = function<void(const list<string>&)>;

class Entity {
private:
    int level;
    int x,y;
    float hp;
    float resistencia;
    string nombrecito;

public:
    Entity(string n, float h, float r, int lv): nombrecito(n), hp(h), resistencia(r), level(lv), x(0), y(0) {}

    void heal(float v){hp+=v; if(hp>100) hp=100;}
    void damage(float v){hp-=v; if(hp<0) hp=0;}
    void useResistencia(float v){resistencia-=v; if(resistencia<0) resistencia=0;}
    void move(int nx,int ny) {x=nx; y=ny;}
    void levelUp() {level++; hp=100; resistencia=100; }
    void reset(){ hp=100; resistencia=100; level=1; x=0; y=0; }

    string getStatus() const {
        return nombrecito + " | HP:" + to_string(hp)
             + " | Resistencia:" + to_string(resistencia)
             + " | Lv:" + to_string(level)
             + " | Pos:(" + to_string(x) + "," + to_string(y) + ")";
    }
    void printStatus() const {cout<<"[STATUS] "<<getStatus()<<"\n";}
};

void freeHeal(Entity& e, const list<string>& args){
    if (args.empty()) { cout<<"[Error] heal necesita 1 argumento.\n"; return; }
    float v=stof(args.front());
    if (v < 0) { cout<<"[Error] heal no puede ser negativo.\n"; return; }
    e.heal(v);
    cout<<"[heal-libre] +" <<v<<" HP\n";}


class DamageFunctor {
    Entity& entity;
    int uses=0;
public:
    DamageFunctor(Entity& e) : entity(e) {}
    void operator()(const list<string>& args) {
        if(args.empty()) { cout << "[Error] damage necesita 1 argumento.\n"; return; }
        float v=stof(args.front());
        if (v<0){ cout<<"[Error] damage no puede ser negativo.\n"; return; }
        uses++;
        entity.damage(v);
        cout <<"[damage-functor] -"<<v<< " HP (uso #" << uses << ")\n";
    }
};

class CommandCenter {
    Entity& entity;
    map<string,Command> commands;
    list<string> history;
    map<string,list<pair<string,list<string>>>>macros;

public:
    CommandCenter(Entity& e):entity(e) {}
    void registerCommand(const string& name,Command cmd) {
        commands[name]=cmd;
        cout<<"[reg] '"<<name<<"' registrado.\n";
    }

    void execute(const string& name, const list<string>& args) {
        map<string,Command>::iterator it=commands.find(name);
        if (it==commands.end()) {
            cout<<"[Error] Comando '"<<name<<"' no existe.\n";
            return;}
        string antes=entity.getStatus();
        it->second(args);
        string despues = entity.getStatus();
        history.push_back("cmd:" + name + " | antes:[" + antes + "] | despues:[" + despues + "]");}

    void removeCommand(const string& name) {
        map<string, Command>::iterator it=commands.find(name);
        if (it==commands.end()) {
            cout<<"[Error] '"<<name<<"' no existe para eliminar.\n";
            return;}
        commands.erase(it);
        cout<<"[rem] '"<<name<<"' eliminado.\n";
    }

    void registerMacro(const string& name, const list<pair<string, list<string>>>& steps) {
        macros[name]=steps;
        cout<<"[macro-reg] '"<<name<<"' registrado.\n";}

    void executeMacro(const string& name) {
        map<string, list<pair<string,list<string>>>>::iterator macroIt = macros.find(name);
        if (macroIt==macros.end()) {
            cout<<"[Error] Macro '"<<name<<"' no existe.\n";
            return;}
        cout<<"[macro] Ejecutando '"<<name<<"'...\n";
        list<pair<string, list<string>>>::iterator stepIt;
        for (stepIt=macroIt->second.begin(); stepIt!=macroIt->second.end(); ++stepIt) {
            if (commands.find(stepIt->first)==commands.end()) {
                cout<<"[Error] Macro detenido: '" <<stepIt->first<<"' no existe.\n";
                return;}
            execute(stepIt->first,stepIt->second);}
    }

    void printHistory() {
        cout<<"HISTORIAL"<<endl;
        list<string>::iterator it;
        for (it=history.begin(); it!=history.end(); ++it)
            cout<<*it<<"\n";}
};
int main() {
    Entity hero("Draven",70.0f,80.0f,2);
    CommandCenter center(hero);

    cout <<"-----Estado Inicial:"<<endl;
    hero.printStatus();

    center.registerCommand("heal", [&hero](const list<string>& args) {
        freeHeal(hero, args);});

    center.registerCommand("move", [&hero](const list<string>& args) {
        if (args.size()<2) {cout<<"[Error] move necesita x e y.\n"; return; }
        list<string>::const_iterator it=args.begin();
        int nx=stoi(*it); ++it;
        int ny=stoi(*it);
        hero.move(nx,ny);
        cout<<"[move-libre] Pos -> (" << nx << "," << ny << ")\n";});

    DamageFunctor dmg(hero);
    center.registerCommand("damage",dmg);

    center.registerCommand("status",[&hero](const list<string>&) {hero.printStatus();});

    center.registerCommand("levelup", [&hero](const list<string>&) {
        hero.levelUp();
        cout << "[levelup-lambda] Nivel subido."<<endl;});

    center.registerCommand("resistencia", [&hero](const list<string>& args) {
        if (args.empty()) { cout << "[Error] resistencia necesita 1 argumento.\n"; return; }
        float v=stof(args.front());
        hero.useResistencia(v);
        cout<<"[resistencia-lambda] -" << v << " resistencia\n";});

    center.registerCommand("reset", [&hero](const list<string>&) {
        hero.reset();
        cout << "[reset-lambda] Entidad reiniciada.\n";});

    cout << "-----Prueba de los Comandos"<<endl;
    center.execute("heal",   {"15"});
    center.execute("heal",   {"30"});
    center.execute("move",   {"10", "5"});

    center.execute("damage", {"20"});
    center.execute("damage", {"10.5"});
    center.execute("damage", {"5"});

    center.execute("status",  {});
    center.execute("levelup", {});
    center.execute("resistencia", {"25"});

    cout << "-----Prueba de los Comandos no validos"<<endl;
    center.execute("heal",{});
    center.execute("move", {"10"});
    center.execute("damage",{"-5"});
    center.execute("volar", {});

    cout << "-----Eliminacion"<<endl;
    center.removeCommand("reset");
    center.execute("reset",{});
    center.removeCommand("reset");

    cout << "-----Macros"<<endl;
    list<pair<string, list<string>>>m1;
    m1.push_back({"heal",{"20"}});
    m1.push_back({"status",{}});
    center.registerMacro("recuperar", m1);

    list<pair<string, list<string>>>m2;
    m2.push_back({"move", {"0","0"}});
    m2.push_back({"levelup",{}});
    m2.push_back({"damage", {"10"}});
    center.registerMacro("iniciar_batalla", m2);

    list<pair<string, list<string>>>m3;
    m3.push_back({"resistencia",{"10"}});
    m3.push_back({"damage",{"15"}});
    m3.push_back({"heal",{"5"}});
    m3.push_back({"status",{}});
    center.registerMacro("turno_combate", m3);

    list<pair<string, list<string>>> m4;
    m4.push_back({"heal",{"10"}});
    m4.push_back({"reset",{}});
    m4.push_back({"status",{}});
    center.registerMacro("macro_roto", m4);
    center.executeMacro("recuperar");
    center.executeMacro("iniciar_batalla");
    center.executeMacro("turno_combate");
    center.executeMacro("macro_roto");
    center.executeMacro("no_existe");
    center.printHistory();
    cout << "-----ESTADO FINAL"<<endl;
    hero.printStatus();

    return 0;
}