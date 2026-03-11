#include <lib.hpp>

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <map>
#include <functional>
#include <limits>


// ChessType 方法实现
ChessType* ChessType::find_target() {
    std::vector<ChessType*>& enemies = game->get_opponent(owner)->chess;
    ChessType* closest = nullptr;
    int min_dist = 100;

    for (ChessType* enemy : enemies) {
        if (enemy->units() <= 0) continue;

        // --- 攻击者与目标的类型匹配逻辑 ---

        // 规则1：如果攻击者是地面单位，而目标是空中单位
        if (this->terrain_type == "land" && enemy->terrain_type == "air") {
            // 只有特定地面单位可以对空，否则跳过此目标
            if (this->name.find("Witch") == std::string::npos &&
                this->name.find("Archer") == std::string::npos &&
                this->name.find("DefenseTower") == std::string::npos) {
                continue;
            }
        }
        
        // 规则2：如果攻击者是气球兵，它不能攻击空中目标
        if (this->name.find("Balloon") != std::string::npos && enemy->terrain_type == "air") {
            continue;
        }

        int dist = abs(position - enemy->position);
        if (dist <= atk_distance && dist < min_dist) {
            min_dist = dist;
            closest = enemy;
        }
    }
    return closest;
}


void ChessType::die() {// 棋子死亡处理
    auto& cell = game->board.board[position];
    cell.erase(std::remove(cell.begin(), cell.end(), this), cell.end());
    std::string color = (owner == game->players[0]) ? BLUE : RED;
    std::cout  << color << name << RESET << " at position " << position << " has died.\n";
}

void ChessType::move() {// 棋子行动逻辑
    if (current_time() - last_move_time >= 0.6 / speed) {// 棋子移动逻辑中的检查环节

        ChessType* target = find_target();
        int dir = (owner == game->players[0]) ? 1 : -1;
        int cur = position;
        int next = cur;

        // 地面单位在当前位置遇到敌方气球或防御塔则停下（防擦肩）
        if (terrain_type == "land") {
            for (ChessType* piece : game->board.board[cur]) {
                if (piece->owner != owner &&
                    (piece->terrain_type == "air" || piece->name == "DefenseTower")) {
                    last_move_time = current_time();
                    return;
                }
            }
        }
        
        //气球AI修正
        if (terrain_type == "air") { // 空中单位移动逻辑
            if (target) { // 如果有目标
                int target_pos = target->position;
                if (position == target_pos) { // 如果已经在目标头顶
                    last_move_time = current_time();
                    // 停止移动，以便进行攻击
                    // 直接返回，不改变位置
                    return; 
                }

                // 计算到目标的距离和方向，确保可以飞向目标，而不是单向前进
                int distance = target_pos - position;
                int move_dir = (distance > 0) ? 1 : -1;

                // 移动步数不能超过自身速度，也不能越过目标
                int move_amount = std::min(speed, std::abs(distance));
                next = position + move_dir * move_amount;

            } else { // 如果没有目标，则继续朝对方基地前进
                for (int i = 0; i < speed; ++i) {
                    int trial = next + dir;
                    if (trial < 0 || trial > 20) break;
                    next = trial;
                }
            }
        } else {// 地面单位逻辑
            if (target && abs(cur - target->position) <= atk_distance) {
                last_move_time = current_time();
                return;
            }

            for (int i = 0; i < speed; ++i) {
                int trial = next + dir;
                if (trial < 0 || trial > 20) break;

                bool enemy_blocking = false;
                for (ChessType* piece : game->board.board[trial]) {
                    // 敌方防御塔阻挡
                    if (piece->name == "DefenseTower" && piece->owner != owner) {
                        enemy_blocking = true;
                        break;
                    }
                    if (piece->owner != owner) {
                        // 阻挡地面单位前进的敌方气球兵
                        if (piece->terrain_type == "air") {
                            enemy_blocking = true;
                            break;
                        }

                        // 敌人在攻击范围内，停止移动
                        if (abs(trial - cur) <= atk_distance) {
                            last_move_time = current_time();
                            return;
                        }

                        // 其他远处阻挡路径的敌人也阻挡移动
                        enemy_blocking = true;
                        break;
                    }
                }
                if (enemy_blocking) break;

                next = trial;
            }
        }


        if (next != cur) {
            auto& cell = game->board.board[cur];
            cell.erase(std::remove(cell.begin(), cell.end(), this), cell.end());
            position = next;
            game->board.board[position].push_back(this);
        }

        last_move_time = current_time();
    }
}



void ChessType::attack() {// 棋子攻击逻辑
    if (current_time() - last_attack_time >= 0.6) {
        for (int i = 0; i < units(); ++i) {
            ChessType* target = find_target();
            if (target) {
                // 颜色显示：自己颜色蓝，敌人红
                std::string my_color = (owner->name == "Player1") ? BLUE : RED;
                std::string target_color = (target->owner->name == "Player1") ? BLUE : RED;

                std::cout << my_color << name << RESET << " at " << position
                          << " attacks "
                          << target_color << target->name << RESET << " at "
                          << target->position << " for "
                          << atk << " damage" << RESET << "\n";

                target->receive_damage(atk, this);
            }
        }
        last_attack_time = current_time();
    }
}

void ChessType::receive_damage(int dmg, ChessType* attacker) {// 棋子受击判定
    if (unit_hp.empty()) return;

    int idx = rand() % unit_hp.size();
    unit_hp[idx] -= dmg;

    std::string self_color = (owner->name == "Player1") ? BLUE : RED;

    std::cout << self_color << name << RESET << "'s unit " << idx
              << " takes " << YELLOW << dmg << RESET
              << " damage, " << YELLOW << "HP now: " << unit_hp[idx] << RESET << "\n";

    if (unit_hp[idx] <= 0) {
        unit_hp.erase(unit_hp.begin() + idx);
    }

    if (unit_hp.empty()) {
        die();
    }
}

// Player 方法实现
void Player::get_money(){// 金币逻辑，增加了一个优劣势的判定，劣势方每两个回合多得一个金币
    if (current_time() - last_money_time >= 1) {
        bool is_disadvantaged = false;
        if (game) {
            if (this == game->players[0]) {
                is_disadvantaged = game->player2_is_advantaged;
            } else {
                is_disadvantaged = game->player1_is_advantaged;
            }
        }
        if (game->round%2 == 0)
            money += is_disadvantaged ? 4 : 3;
        else
            money += 3;
        last_money_time = current_time();
    }
}

// Game 方法实现
void Game::display_board() {// GLI界面显示
    std::cout << "\n======= Round " << round << " =======\n\n";
    std::cout << "Chessboard (P=Player1, E=Player2 or AI):\n";
    std::cout << BLUE << "Blue" << RESET << " = Player1    "
              << RED << "Red" << RESET << " = Player2 / AI\n";

    // 显示空中单位（Sky）
    std::cout << "\n[Sky]:\n";
    for (int layer = 1 ;layer >= 0; --layer) {
         for (int i = 0; i < 21; ++i) {
            if (i == 10) std::cout << " | ";

            // 统计每个名字+玩家组合的空中单位总数
            std::map<std::pair<std::string, Player*>, int> sky_count;
            for (ChessType* unit : board.board[i]) {
                if (unit->terrain_type == "air" && unit->units() > 0 && unit->name.find("DefenseTower") == std::string::npos) {
                    std::pair<std::string, Player*> key = {unit->name, unit->owner};
                    sky_count[key] += unit->units();
                }
            }

            std::string cell_display = "";

            std::vector<std::string> display_units;
            for (const auto& pair : sky_count) {
                const auto& key = pair.first;
                int total_units = pair.second;

                std::string name_to_show = key.first;
                char initial = name_to_show[0];
                if (name_to_show.find("BabyDragon") != std::string::npos) initial = 'D';
                if (name_to_show.find("Minions") != std::string::npos) initial = 'M'; // 为亡灵也添加缩写
                

                Player* owner = key.second;
                std::ostringstream oss;
                oss << initial << "(" << total_units << ")";
                std::string padded = oss.str();
                while (padded.length() < 6) padded += " ";
                std::string color = (owner == players[0]) ? BLUE : RED;
                display_units.push_back(color + padded + RESET);
            }

            // 根据当前 layer 从 vector 中取出要显示的内容
            if (layer < display_units.size()) {
                cell_display = display_units[layer];
            }

            std::cout << std::setw(6) << std::left << cell_display;
        }

        std::cout << "\n";
    }
   

    // 防御塔位置和图案
    const std::vector<int> tower_positions = {2, 18};
    const std::string tower_graphic[3] = {"  #  ", " ### ", "#####"};

    // 打印地面单位 + 防御塔图形
    for (int layer = 3; layer >= 0; --layer) {
        for (int i = 0; i < 21; ++i) {
            if (i == 10) std::cout << " | ";

            std::string cell_display = "      ";
            bool is_tower_here = std::find(tower_positions.begin(), tower_positions.end(), i) != tower_positions.end();

            bool tower_alive = false;
            int owner_idx = -1;
            for (ChessType* unit : board.board[i]) {
                if (unit->name.find("DefenseTower") != std::string::npos && unit->units() > 0) {
                    tower_alive = true;
                    owner_idx = (unit->owner == players[0]) ? 0 : 1;
                    break;
                }
            }

            std::string tower_color = (owner_idx == 0) ? BLUE : RED;

            if (is_tower_here && tower_alive && layer <= 2) {
                std::string raw = tower_graphic[2 - layer];
                int pad_left = (6 - raw.size()) / 2;
                std::string padded = std::string(pad_left, ' ') + raw + std::string(6 - pad_left - raw.size(), ' ');
                cell_display = tower_color + padded + RESET;
            } else {
                std::vector<ChessType*>& cell = board.board[i];
                std::map<std::pair<std::string, Player*>, int> count_map;

                for (ChessType* unit : cell) {
                    if (unit->units() > 0 && unit->terrain_type == "land" && unit->name.find("DefenseTower") == std::string::npos) {
                        std::pair<std::string, Player*> key = {unit->name, unit->owner};
                        count_map[key] += unit->units();
                    }
                }

                std::vector<std::string> display_units;
                for (auto& [key, total_units] : count_map) {
                    std::string name = key.first;
                    Player* owner = key.second;
                    char initial = name[0];
                    if (name.find("P_E_K_K_A") != std::string::npos) initial = 'P';
                    if (name.find("Prince") != std::string::npos) initial = 'R'; // 将王子的缩写强制设置为 'R'
                    std::ostringstream oss;
                    oss << initial << "(" << total_units << ")";
                    std::string padded = oss.str() + std::string(6 - oss.str().length(), ' ');
                    std::string color = (owner == players[0]) ? BLUE : RED;
                    display_units.push_back(color + padded + RESET);
                }

                if (layer < display_units.size()) {
                    cell_display = display_units[layer];
                }
            }

            std::cout << std::setw(6) << std::left << cell_display;
        }
        std::cout << '\n';
    }

    // 分隔线
    std::cout << std::string(132, '=') << '\n';

    // 显示格子编号
    for (int i = 0; i < 21; ++i) {
        if (i == 10) std::cout << " |  ";
        std::cout << std::setw(6) << std::left << i;
    }
    std::cout << '\n';

    // 防御塔状态栏
    std::cout << "\nDefense Tower Status:  ";
    for (int p = 0; p < 2; ++p) {
        std::string color = (p == 0) ? BLUE : RED;
        std::cout << color << players[p]->name << RESET << ": ";
        bool tower_found_alive = false;
        for (int i : tower_positions) {
            for (ChessType* u : board.board[i]) {
                if (u->name.find("DefenseTower") != std::string::npos && u->units() > 0 && u->owner == players[p]) {
                    std::cout << u->unit_hp[0] << " HP  ";
                    tower_found_alive = true;
                    break;
                }
            }
            if(tower_found_alive) break;
        }
        if (!tower_found_alive) {
            std::cout << YELLOW << "Destroyed  " << RESET;
        }
    }
    std::cout << std::endl;
}


void Game::spawn_phase() {
    for (auto* player : players) {
        player->get_money();
        std::cout << player->name << " (money: " << player->money << ")\n";
        if (player->is_ai) {
            // AI行动逻辑
            if (player->money < 4 && (rand() % 10) < 5) {
                std::cout << "AI decides to save money this turn.\n";
                continue;
            }

            // 更新后的克制关系表
            std::map<std::string, std::string> counter = {
                {"Skeleton", "BabyDragon"}, {"Archer", "Prince"}, {"Knight", "P_E_K_K_A"},
                {"P_E_K_K_A", "Skeleton"}, {"P_E_K_K_A", "Minions"}, {"P_E_K_K_A", "Goblin"},
                {"Witch", "Prince"}, {"Balloon", "BabyDragon"}, {"Balloon", "Minions"}, {"Balloon", "Archer"},
                {"Minions", "BabyDragon"}, {"Goblin", "Witch"}, {"Goblin", "BabyDragon"},
                {"Prince", "Skeleton"}, {"BabyDragon", "Archer"}
            };

            auto* opponent = get_opponent(player);
            std::vector<std::string> enemy_buys = opponent->recent_buys;

            struct ScoredChoice { std::function<ChessType*()> factory; double score; std::string name; };
            std::vector<ScoredChoice> options;

            for (auto& f : chess_pool) {
                ChessType* u = f();
                if (u->get_cost() > player->money) { delete u; continue; }

                double bonus = 0;
                for (auto& e : enemy_buys) {
                    if (counter[e] == u->name) bonus += 25;
                }

                double score = u->atk * u->units() / (double)u->get_cost() + u->get_atk_distance() * 7 + bonus + ((rand() % 10) - 5);

                // 根据AI策略调整分数
                switch(player->strategy) {
                    case Player::AIStrategy::Aggressive:
                        score += u->atk * 1.5 + u->get_speed() * 1.0;
                        if (u->terrain_type == "air") score += 15;
                        break;
                    case Player::AIStrategy::Swarm:
                        if (u->units() > 1) score += u->units() * 15;
                        if (u->get_cost() <= 3) score += 20;
                        break;
                    case Player::AIStrategy::Defensive:
                        score += u->get_atk_distance() * 10;
                        if (u->name == "Witch" || u->name == "Archer") score += 25;
                        break;
                    case Player::AIStrategy::Balanced:
                        // No changes for balanced
                        break;
                }

                options.push_back({f, score, u->name});
                delete u;
            }

            if (options.empty()) {
                std::cout << "AI has no affordable chess pieces to buy.\n";
            } else {
                std::sort(options.begin(), options.end(), [](const ScoredChoice& a, const ScoredChoice& b) { return a.score > b.score; });

                while (true) {
                    bool bought_any = false;
                    for (auto& opt : options) {
                        ChessType* unit = opt.factory();
                        if (unit->get_cost() <= player->money) {
                            unit->owner = player;
                            unit->game = this;
                            unit->position = (player == players[0]) ? 0 : 20;
                            board.board[unit->position].push_back(unit);
                            player->money -= unit->get_cost();
                            player->add_chess(unit);
                            std::cout << "AI buys " << unit->name << " (cost: " << unit->get_cost() << ", remaining gold: " << player->money << ")\n";
                            bought_any = true;
                            // Re-sort options based on remaining money
                            goto next_buy_cycle;
                        }
                        delete unit;
                    }
                    next_buy_cycle:
                    if (!bought_any) break;
                }
            }

            // AI升级逻辑
            if (upgrades_enabled && round % 2 == 0 && player->money > 3) {
                std::vector<ChessType*> upgradable_units;
                for(auto* unit : player->chess) {
                    if (!unit->upgraded && unit->get_cost() <= player->money && unit->name.find("DefenseTower") == std::string::npos) {
                        upgradable_units.push_back(unit);
                    }
                }

                if (!upgradable_units.empty()) {
                    std::sort(upgradable_units.begin(), upgradable_units.end(), [](ChessType* a, ChessType* b){ return a->get_cost() > b->get_cost(); });
                    ChessType* to_upgrade = upgradable_units[0];

                    player->money -= to_upgrade->get_cost();
                    to_upgrade->atk += 10;
                    for (int& hp : to_upgrade->unit_hp) hp += to_upgrade->unit_hp[0] + 10;
                    to_upgrade->name += "+";
                    to_upgrade->upgraded = true;

                    std::cout << "AI upgrades " << to_upgrade->name << " (cost: " << to_upgrade->get_cost() << ", remaining gold: " << player->money << ")\n";
                }
            }
        }
        
        else {  // 普通玩家
            char response;
            std::cout << "Do you want to buy a chess piece (y/n): ";
            std::cin >> response;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            if (response == 'y') {
                // 显示商店
                std::cout << "Available pieces:\n";
                for (int i = 0; i < chess_pool.size(); ++i) {
                    ChessType* u = chess_pool[i]();
                    std::cout << i + 1 << ". " << u->name << " (" << u->get_cost() << " gold)"
                            << " \t[" << u->units() << " units, HPs: ";
                    for (int h : u->unit_hp) std::cout << h << " ";
                    std::cout << ", " << u->atk << " atk, " << u->get_atk_distance()
                            << " atk_distance, " << u->get_speed() << " speed, "
                            << u->terrain_type<< "]\n";
                    delete u;
                }

                bool valid = false;
                while (!valid) {
                    std::cout << "Enter the numbers of the chess pieces you want to buy (space-separated): ";
                    std::string line;
                    std::getline(std::cin, line);

                    std::istringstream iss(line);
                    std::vector<int> choices;
                    int choice;
                    bool input_error = false;
                    int total_cost = 0;

                    // 先计算总价格
                    while (iss >> choice) {
                        if (choice < 1 || choice > chess_pool.size()) {
                            std::cout << "Invalid choice: " << choice << "\n";
                            input_error = true;
                            break;
                        }
                        ChessType* u = chess_pool[choice - 1]();
                        total_cost += u->get_cost();
                        delete u;
                        choices.push_back(choice);
                    }

                    if (input_error) continue;

                    if (total_cost > player->money) {
                        std::cout << "Not enough gold! Total cost: " << total_cost << ", you have: " << player->money << "\n";
                        continue;
                    }

                    // 合法输入，开始购买
                    for (int idx : choices) {
                        ChessType* u = chess_pool[idx - 1]();
                        u->owner = player;
                        u->game = this;
                        u->position = (player == players[0]) ? 0 : 20;
                        board.board[u->position].push_back(u);
                        player->money -= u->get_cost();
                        player->add_chess(u);
                        std::cout << "Bought " << u->name << " for " << u->get_cost() << " gold. Remaining: " << player->money << "\n";
                    }

                    valid = true; // 成功退出循环
                }
            }

            // 升级逻辑，只有偶数回合才能升级，并且每个棋子只能升级一次
           if (upgrades_enabled && round % 2 == 0) {
                while (true) {
                    char upgrade_response;
                    std::cout << "Do you want to upgrade a chess piece? (y/n): ";
                    std::cin >> upgrade_response;
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    if (upgrade_response != 'y') break;

                    std::vector<ChessType*> upgradable;
                    for (ChessType* unit : player->chess) {
                        if (!unit->unit_hp.empty() && !unit->upgraded && unit->name.find("DefenseTower") == std::string::npos) {
                            upgradable.push_back(unit);
                        }
                    }

                    if (upgradable.empty()) {
                        std::cout << "No units available for upgrade.\n";
                        break;
                    } 
                    
                    std::cout << "Select a piece to upgrade:\n";
                    for (int i = 0; i < upgradable.size(); ++i) {
                        ChessType* unit = upgradable[i];
                        std::cout << i + 1 << ". " << unit->name << " at " << unit->position
                                  << " (Upgrade cost: " << unit->get_cost() << ")\n";
                    }
                    
                    std::cout << "Enter your choice: ";
                    int choice;
                    std::cin >> choice;
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

                    if (choice < 1 || choice > upgradable.size()) {
                        std::cout << "Invalid choice.\n";
                    } else {
                        ChessType* unit = upgradable[choice - 1];
                        if (player->money < unit->get_cost()) {
                            std::cout << "Not enough gold! You need " << unit->get_cost() << ".\n";
                            break;
                        } else {
                            player->money -= unit->get_cost();
                            unit->atk += 10;
                            for (int& hp : unit->unit_hp) hp = unit->unit_hp[0] + 10;
                            unit->name += "+";
                            unit->upgraded = true;
                            std::cout << unit->name << " has been upgraded! ATK +10, Max HP +10, Restored to full health.\n";
                        }
                    }
                    if (player->money <= 0) {
                        std::cout << "No more gold for upgrades.\n";
                        break;
                    }
                }
            }
        }
    }
}

void Game::update_advantage_status() {
    int p1_max_pos = -1;
    int p2_min_pos = 100;

    for (auto* c : players[0]->chess) {
        if (c->units()  > 0) p1_max_pos = std::max(p1_max_pos, c->position);
    }
    for (auto* c : players[1]->chess) {
        if (c->units()  > 0) p2_min_pos = std::min(p2_min_pos, c->position);
    }

    // 判断是否越过中线（中线位置为10）
    if (p1_max_pos > 10) {
        player1_frontline_counter++;
    } else {
        player1_frontline_counter = 0;
    }

    if (p2_min_pos < 10) {
        player2_frontline_counter++;
    } else {
        player2_frontline_counter = 0;
    }

    // 设置优势方
    player1_is_advantaged = (player1_frontline_counter >= 2);
    player2_is_advantaged = (player2_frontline_counter >= 2);
}

void Game::update_phase() {// 更新阶段
    // 在更新阶段，程序在调用棋子的移动和攻击方法时，可能访问了已经死亡并被删除的棋子，导致悬空指针问题所以出现程序输出四轮后异常终止
    // 收集所有活着的棋子（避免 move/attack 调用已死亡单位）
    std::vector<ChessType*> living_units;
    for (auto* p : players) {
        for (auto* c : p->chess) {
            if (!c->unit_hp.empty()) {
                living_units.push_back(c);
            }
        }
    }

    // 按顺序逐个行动，行动前检查是否还活着
    for (ChessType* c : living_units) {
        if (c->unit_hp.empty()) continue;  // 避免死者再行动

        c->move();

        if (!c->unit_hp.empty()) {
           c->attack();
        }
    }

    // 清理死亡单位
    for (auto* p : players) {
        auto& ch = p->chess;
        ch.erase(std::remove_if(ch.begin(), ch.end(), [](ChessType* unit) {
            if (unit->unit_hp.empty()) {
                auto& cell = unit->game->board.board[unit->position];
                cell.erase(std::remove(cell.begin(), cell.end(), unit), cell.end());
                delete unit;
                return true;
            }
            return false;
        }), ch.end());
    }

    if (debug_mode) {
        for (auto* p : players) {
            std::cout << "[DEBUG] " << p->name << " units:\n";
            for (auto* c : p->chess) {
                std::cout << "  - " << c->name << " at " << c->position << " (Units: " << c->units() << ")\n";
            }
        }
    }


    update_advantage_status();
}

bool Game::check_victory() {
    if (debug_mode) std::cout << "[DEBUG] Checking victory conditions...\n";

    for (auto* c : players[0]->chess) {
        if (debug_mode) std::cout << "[DEBUG] P1 " << c->name << " at " << c->position << " (HP: " << c->units() << ")\n";
        if (c->position == 20) {
            std::cout << "Player 1 wins\n";
            return true;
        }
    }

    for (auto* c : players[1]->chess) {
        if (debug_mode) std::cout << "[DEBUG] P2 " << c->name << " at " << c->position << " (HP: " << c->units()  << ")\n";
        if (c->position == 0) {
            std::cout << players[1]->name << " wins\n";
            return true;
        }
    }

    for (auto* c : players[0]->chess)
        if (c->position == 20) {
            //std::cout << "[DEBUG] Victory condition met: " << c->name << " of Player 1 at position 20\n";
            std::cout << "Player 1 wins\n";
            return true;
        }
    for (auto* c : players[1]->chess)
        if (c->position == 0) {
            //std::cout << "[DEBUG] Victory condition met: " << c->name << " of AI at position 0\n";
            std::cout << players[1]->name << " wins\n";
            return true;
        }
    return false;
}

void Game::start() {
    srand(time(0));
    chess_pool = { [](){ return new Skeleton(); }, [](){ return new Knight(); },
                   [](){ return new Archer(); }, [](){ return new P_E_K_K_A(); }, 
                   [](){ return new Witch(); }, [](){ return new Balloon(); },
                   [](){ return new Minions(); } ,[](){ return new Goblin(); },
                   [](){ return new Prince(); },[](){ return new BabyDragon(); }}; 

    // 初始化防御塔位置（P1: 2，P2: 18）
    DefenseTower* tower1 = new DefenseTower(players[0]);
    tower1->position = 2;
    tower1->game = this;
    players[0]->add_chess(tower1);
    board.board[2].push_back(tower1);

    DefenseTower* tower2 = new DefenseTower(players[1]);
    tower2->position = 18;
    tower2->game = this;
    players[1]->add_chess(tower2);
    board.board[18].push_back(tower2);

    while (true) {
        ++round;
        display_board();
        spawn_phase();
        update_phase();
        if (check_victory()) break;
    }
}