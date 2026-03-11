// Header guard to avoid #include the same header more than once
// 类的定义（包含一些基本函数）与函数的声明
// 最好不要写using namespace std;
#ifndef EXAMPLE_LIB_HEADER
#define EXAMPLE_LIB_HEADER

#define RESET   "\033[0m"// 重置颜色
#define RED     "\033[31m"
#define BLUE    "\033[34m"
#define YELLOW  "\033[33m"

#include <iostream>
#include <vector>
#include <string>
//#include <memory>
#include <ctime>
//#include <cstdlib>
//#include <cmath>
//#include <algorithm>
//#include <map>
#include <functional>

inline double current_time();

class Game;
class Player;
class ChessType;

// 时间函数，将CPU时间转换为秒
inline double current_time() {
    return static_cast<double>(clock()) / CLOCKS_PER_SEC;
}


// 棋子抽象基类
class ChessType {
private:
    int atk_distance, speed, cost;// 基本属性    
public:
    int atk;
    int get_atk_distance() const { return atk_distance; }
    int get_speed() const { return speed; }
    int get_cost() const { return cost; }
    std::string name;// 棋子名字
    std::string terrain_type;// 区分是空中单位还是陆地单位，有land和air两个取值
    int position;// 位置
    Player* owner;// 所属下棋者
    Game* game;// 所属游戏
    double last_move_time;// 上次移动时间
    double last_attack_time;// 上次攻击时间
    bool upgraded = false;// 是否升级过
    
    std::vector<int> unit_hp; // 每个unit的独立hp，为多单位棋子而设计

    ChessType(std::string n, int unit_count, int per_unit_hp, int a, int ad, int s, int c, std::string t)
        : name(n), atk(a), atk_distance(ad), speed(s), cost(c), terrain_type(t),
          position(-1), owner(nullptr), game(nullptr),
          last_move_time(current_time()), last_attack_time(current_time()) {
            unit_hp = std::vector<int>(unit_count, per_unit_hp);
          }// 构造函数

    virtual ~ChessType() {}// 析构函数

    int units() const { return unit_hp.size(); }

    ChessType* find_target();// 寻找最近的目标
    virtual void move();
    virtual void attack();
    virtual void die();
    virtual void receive_damage(int damage, ChessType* attacker);
};

class AirUnit : public ChessType {
public:
    // 构造函数，直接调用父类ChessType的构造函数
    using ChessType::ChessType;

    // 重写受伤处理逻辑
    void receive_damage(int damage, ChessType* attacker) override{
        if (!attacker) return; // 如果没有攻击者信息，则不受伤害

        // 定义可以对空的单位名称列表
        const std::vector<std::string> valid_attackers = {
            "Witch", "Archer", "Minions", "BabyDragon", "DefenseTower"
        };

        bool can_be_attacked = false;
        for (const auto& valid_name : valid_attackers) {
            // 使用 find 来检查，这样 "Witch+" 这样的升级单位也能被正确识别
            if (attacker->name.find(valid_name) != std::string::npos) {
                can_be_attacked = true;
                break;
            }
        }

        // 如果攻击者不合法，则直接返回，不造成任何伤害
        if (!can_be_attacked) {
            return;
        }

        // 如果攻击者合法，则调用基类（ChessType）的通用受伤逻辑
        ChessType::receive_damage(damage, attacker);
    };
};

// 棋盘类
class ChessBoard {
public:
    std::vector<std::vector<ChessType*>> board;
    ChessBoard() { board.resize(21); }
};

// 玩家类
class Player {
private:
    
public:
    std::string name;// 玩家名
    int money;// 金币
    std::vector<ChessType*> chess;// 棋子
    double last_money_time;// 上次获得金币时间
    bool is_ai;// 区分AI和人类玩家
    Game* game;// 所属游戏
    std::vector<std::string> recent_buys;// 最近购买的棋子

    // AI策略枚举
    enum class AIStrategy { Balanced, Aggressive, Swarm, Defensive };
    AIStrategy strategy;


    Player(std::string n, bool ai = false)
        : name(n), money(n == "Player1" ? 10 : 7), last_money_time(current_time()), is_ai(ai), game(nullptr) {
            if (is_ai) {
                // 为AI随机选择一个策略
                strategy = static_cast<AIStrategy>(rand() % 4);
            }
        }


    void get_money() ;

    void add_chess(ChessType* c){
        chess.push_back(c);
        recent_buys.push_back(c->name);
        if (recent_buys.size() > 5) recent_buys.erase(recent_buys.begin());
    } ;

};

// 游戏类
class Game {
public:
    std::vector<Player*> players;// 玩家
    ChessBoard board;// 棋盘
    int round = 0;// 回合
    std::vector<std::function<ChessType*()>> chess_pool;// 棋子池
    bool debug_mode = false ; // 调试模式
    bool upgrades_enabled ; // 是否启用升级
    Game(bool vs_ai = true, bool enable_upgrades = true) : upgrades_enabled(enable_upgrades) {
        players.push_back(new Player("Player1"));
        players.push_back(new Player(vs_ai ? "AI" : "Player2", vs_ai));
        for (auto& p : players) p->game = this;
    }

    Player* get_opponent(Player* player) {//简单内置函数，返回对手的指针
        return player == players[0] ? players[1] : players[0];
    }
    int player1_frontline_counter = 0;// 优势计数
    int player2_frontline_counter = 0;// 优势计数
    bool player1_is_advantaged = false;// 优势
    bool player2_is_advantaged = false;// 优势
    void display_board();
    void spawn_phase();
    void update_phase();
    bool check_victory();
    void start();
    void update_advantage_status();
};

// 棋子类型定义
class Skeleton : public ChessType {// 普通骷髅
public:
    Skeleton() : ChessType("Skeleton", 3, 15, 10, 1, 3, 1,"land") {}
};

class Skeleton_by_witch : public ChessType {// 女巫生成的骷髅
public:
    Skeleton_by_witch() : ChessType("Skeleton_by_witch", 2, 16, 8, 1, 3, 0, "land") {}
};

class Knight : public ChessType {// 骑士
public:
    Knight() : ChessType("Knight", 1, 120, 18, 1, 2, 3,  "land") {}
};

class Archer : public ChessType {// 弓箭手
public:
    Archer() : ChessType("Archer", 2, 40, 15, 3, 3, 3, "land") {}
};

class P_E_K_K_A : public ChessType {// 皮卡超人
public:
    int attack_counter = 0;
    P_E_K_K_A() : ChessType("P_E_K_K_A", 1, 320, 100, 1, 1, 6, "land") {}

    void attack() override {
        if (find_target()) { // 只有找到目标时才会计数
            attack_counter++;
            if (attack_counter % 2 != 0) { // 如果是奇数回合，则跳过攻击
                last_attack_time = current_time();
                return;
            }
        }
        ChessType::attack(); // 偶数回合执行普通攻击
    }
};

class Witch : public ChessType {// 女巫
public:
    double summon_timer;
    Witch() : ChessType("Witch", 1, 80, 20, 2, 2, 4, "land"), summon_timer(current_time()) {}

    void attack() override {// 女巫的攻击逻辑，每秒生成一个骷髅
        ChessType::attack();
        if (current_time() - summon_timer >= 1.0) {
            int offset = (owner == game->players[0]) ? 1 : -1;
            int pos = position + offset;
            if (pos >= 0 && pos < 21) {
                Skeleton_by_witch* s = new Skeleton_by_witch();
                s->position = pos;
                s->owner = owner;
                s->game = game;
                owner->add_chess(s);
                game->board.board[pos].push_back(s);
            }
            summon_timer = current_time();
        }
    }
};

class Balloon : public AirUnit {// 飞行单位
public:
    Balloon() : AirUnit("Balloon", 1, 150, 30, 0, 2, 4, "air") {}


    void attack() override {// 特殊的AOE伤害
        if (current_time() - last_attack_time >= 0.6) {
            std::vector<ChessType*>& enemies = game->get_opponent(owner)->chess;
            for (ChessType* e : enemies) {
                if (e->units() <= 0) continue;

                int dist = abs(e->position - position);
                if (dist <= 0 && e->terrain_type == "land") {
                    std::cout << name << " at " << position << " drops bomb on "
                            << e->name << " at " << e->position << "!\n";
                    e->receive_damage(atk, this);
                    e->receive_damage(atk, this);
                }
            }
            last_attack_time = current_time();
        }
    }

    void die() override {// 亡语
        std::cout << name << " at " << position << " explodes!\n";
        std::vector<ChessType*>& enemies = game->get_opponent(owner)->chess;
        for (ChessType* e : enemies) {
            // 加入以下检查条件：敌人在同一格，地面单位，且还有存活单位
            if (e->position == position && e->terrain_type == "land" && e->units() > 0) {
                std::cout << "  -> Explosion hits " << e->name << "!\n";
                e->receive_damage(60, this);
            }
        }
        ChessType::die();
    }
};

class DefenseTower : public ChessType {// 防御塔
public:
    DefenseTower(Player* owner)
        : ChessType("DefenseTower", 1, 800, 30, 3, 0, 0, "land") {
        this->owner = owner;
    }

    void move() override {
        // 防御塔不移动
    }

    void attack() override {
        if (current_time() - last_attack_time >= 0.6) {
            ChessType* target = find_target();
            if (target) {
                std::cout << name << " at " << position
                          << " shoots at " << target->name
                          << " at " << target->position << "!\n";
                target->receive_damage(atk, this);
            }
            last_attack_time = current_time();
        }
    }

    void die() override {
        std::cout << (owner == game->players[0] ? BLUE : RED)
                  << "Defense Tower has been destroyed!\n" << RESET;
        ChessType::die();
    }
};

class Minions : public AirUnit {// 亡灵
public:
    Minions() : AirUnit("Minions", 3, 20, 15, 3, 3, 3, "air") {} 
};

class Goblin : public ChessType { // 哥布林
public:
    Goblin() : ChessType("Goblin", 3, 30, 12, 1, 4, 2, "land") {}
};

class Prince : public ChessType { // 王子
public:
    bool charged = true; // 出场即带冲锋
    Prince() : ChessType("Prince", 1, 200, 35, 1, 3, 5, "land") {}

    void move() override {// 冲锋
    int original_pos = position;
    ChessType::move(); 
        if (position != original_pos) {
            charged = true; 
        }
    };

    void attack() override {
        if (current_time() - last_attack_time >= 0.6) {
            ChessType* target = find_target();
            if (target) {
                int damage = charged ? atk * 2 : atk;
                if (charged) {
                    std::cout << "Prince CHARGES and attacks for " << damage << " damage\n";
                    charged = false; // 消耗冲锋
                }
                target->receive_damage(damage, this);
            }
            last_attack_time = current_time();
        }
    }
};

class BabyDragon : public AirUnit { // 飞龙宝宝
public:
    BabyDragon() : AirUnit("BabyDragon", 1, 150, 15, 3, 3, 4, "air") {}


    // 在 BabyDragon 类中，完全替换旧的 attack() 函数
    void attack() override {
        if (current_time() - last_attack_time >= 0.6) {
            // 获取所有敌人单位的列表
            std::vector<ChessType*>& enemies = game->get_opponent(owner)->chess;
            bool attacked = false; // 标记本回合是否已攻击

            std::string my_color = (owner->name == "Player1") ? BLUE : RED;
            std::cout << my_color << name << RESET << " at " << position << " breathes fire!\n";

            // 遍历所有敌人
            for (ChessType* enemy : enemies) {
                // 跳过已死亡的单位
                if (enemy->units() <= 0) continue;

                // 计算与敌人的距离
                int dist = abs(enemy->position - position);

                if (dist <= get_atk_distance()) {
                    // 根据规则，飞龙宝宝可以攻击地面和空中单位，这里不需要额外判断
                    enemy->receive_damage(atk, this);
                    attacked = true; // 确认发起了攻击
                }
            }

            // 只有在至少攻击了一个目标后才更新攻击计时器
            if (attacked) {
                last_attack_time = current_time();
            }
        }
    }
};

#endif