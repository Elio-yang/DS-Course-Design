/*
 * @author Elio Yang
 * @email  jluelioyang2001@gamil.com
 * @date 2021/3/28
 */
#include <iostream>
#include <cstdio>
#include <string>
#include <cstring>
#include <cmath>
#include <ctime>
#include <random>
#include "jsoncpp/json.h"

using namespace std;

struct P {
public:
        int x;
        int y;

        P(int xx, int yy) : x(xx), y(yy)
        {}
};

//size = 32
vector<P> range = {
        {0, 0},
        {0, 1},
        {0, 2},
        {0, 3},
        {0, 4},
        {0, 5},
        {0, 6},
        {0, 7},
        {0, 8},
        {1, 0},
        {2, 0},
        {3, 0},
        {4, 0},
        {5, 0},
        {6, 0},
        {7, 0},
        {8, 0},
        {1, 8},
        {2, 8},
        {3, 8},
        {4, 8},
        {5, 8},
        {6, 8},
        {7, 8},
        {8, 8},
        {8, 1},
        {8, 2},
        {8, 3},
        {8, 4},
        {8, 5},
        {8, 6},
        {8, 7}
};


#define EMPTY 0
#define BLACK (1)
#define WHITE (-1)

//#define DEBUG
//#define SHOW_BOARD
//#define TIME
#define RUN
//#define LOCAL

#define InBoard for(int i=0;i<9;i++)        \
                                        for(int j=0;j<9;j++)
#define  TL 0.94
constexpr double max_double = (2e50);
constexpr double min_double = -max_double;
constexpr auto scale = 100.0;
int time_allowed = (int) (TL * (double) CLOCKS_PER_SEC);
int montecarlo_nodes = 0;
int other_x;
int other_y;
typedef enum {
        up = 0,
        down,
        l,
        r
} direction;
/*------------------------------Basic tools provided---------------------------------------*/
int board[9][9] = {{0}};
bool dfs_air_visit[9][9];
const int cx[] = {-1, 0, 1, 0};
const int cy[] = {0, -1, 0, 1};

bool inBorder(int x, int y)
{
        return x >= 0 && y >= 0 && x < 9 && y < 9;
}

/*-------------------------------Some tools------------------------------------------------*/
inline unsigned int get_random(int n)
{
        uniform_int_distribution<unsigned> src(0, n - 1);
        default_random_engine e(time(nullptr));
        return src(e);
}

inline bool is_boundary(int x, int y)
{
        return (x == 0 || y == 0 || x == 8 || y == 8);
}
/*--------------------------------Debug part----------------------------------------------*/

#define show_current_board(current_board, mesg) ({\
        printf("%s\n", mesg);\
        for (int i = 0; i < 9; i++) {\
                for (int j = 0; j < 9; j++) {\
                        printf("[%c]", (current_board [j] [i] == BLACK) ? ('#') : ( current_board[j][i]==WHITE ? ('@'):(' ')));\
                }\
                printf("\n");\
        }\
})

/*------------------Monte-Carlo search tree with CUB1 part--------------------------------*/
class MontecarloState {
public:
        int turnID;
        int who;
        int current_board[9][9] = {0};
        vector<P> choice_set;

        static bool dfs_some_board_air(int fx, int fy, int(*some_board)[9]);

        static bool judge_some_board_Available(int fx, int fy, int col, int(*some_board)[9]);

        inline void put(P p)
        {
                if (inBorder(p.x, p.y)) {
                        current_board[p.x][p.y] = who;
                }
        }

        inline void switch_player()
        {
                ++turnID;
                who = -who;
        }

        inline double random_gameing_score()
        {
                int v_m = 0, v_o = 0;
                InBoard {
                                if (!judge_some_board_Available(i, j, -who, current_board)) {
                                        v_m++;
                                }
                        }
                InBoard {
                                if (!judge_some_board_Available(i, j, who, current_board)) {
                                        v_o++;
                                }
                        }
                return ((v_o - v_m)) / scale;
        }


        void *next_random_state()
        {
                if (choice_set.empty()) {
                        return nullptr;
                }
                int random = get_random(choice_set.size());
                put(choice_set[random]);
                switch_player();
                get_all_choice();
        }

        void get_all_choice()
        {
                choice_set.clear();
                for (int i = 0; i < 9; i++) {
                        for (int j = 0; j < 9; j++) {
                                if (judge_some_board_Available(i, j, who, current_board)) {
                                        choice_set.push_back({i, j});
                                }
                        }
                }
        }

        void load_board()
        {
                for (int i = 0; i < 9; i++)
                        for (int j = 0; j < 9; j++)
                                current_board[i][j] = board[i][j];
        }

        MontecarloState(int player, int id)
        {
                who = player;
                turnID = id;
                get_all_choice();
        };

        // default policy to get value of this state
        double default_policy()
        {
                return random_gameing_score();
        }

};

bool MontecarloState::dfs_some_board_air(int fx, int fy, int(*some_board)[9])
{
        dfs_air_visit[fx][fy] = true;
        bool flag = false;
        for (int dir = 0; dir < 4; dir++) {
                int dx = fx + cx[dir], dy = fy + cy[dir];
                if (inBorder(dx, dy)) {
                        if (some_board[dx][dy] == 0)
                                flag = true;
                        if (some_board[dx][dy] == some_board[fx][fy] && !dfs_air_visit[dx][dy])
                                if (dfs_some_board_air(dx, dy, some_board))
                                        flag = true;
                }
        }
        return flag;
}

bool MontecarloState::judge_some_board_Available(int fx, int fy, int col, int(*some_board)[9])
{
        if (some_board[fx][fy]) return false;//若此处已放棋子，则决不能再放棋子
        some_board[fx][fy] = col;
        memset(dfs_air_visit, 0, sizeof(dfs_air_visit));
        if (!dfs_some_board_air(fx, fy, some_board)) {
                some_board[fx][fy] = 0;
                return false;
        }
        for (int dir = 0; dir < 4; dir++) {
                int dx = fx + cx[dir], dy = fy + cy[dir];
                if (inBorder(dx, dy)) {
                        if (some_board[dx][dy] && !dfs_air_visit[dx][dy])
                                if (!dfs_some_board_air(dx, dy, some_board)) {
                                        some_board[fx][fy] = 0;
                                        return false;
                                }
                }
        }
        some_board[fx][fy] = 0;
        return true;
}

class MontecarloNode {
public:

        MontecarloNode *first, *next, *parent;
        vector<MontecarloNode *> children;
        MontecarloState state = MontecarloState(0, 0);
        double V;
        int N;

        MontecarloNode(MontecarloNode *p) : parent(p), first(nullptr), next(nullptr), V(0.0), N(0)
        {
        }

        ~MontecarloNode()
        {
                MontecarloNode *ptr;
                MontecarloNode *this_node = first;
                while (this_node != nullptr) {
                        ptr = this_node->next;
                        delete this_node;
                        this_node = ptr;
                }
                children.clear();
        }

        /*--------------------------------------------------------------------------------------------------------*/
        void add_value(double v)
        {
                this->V += v;
                ++this->N;
        }

        inline bool is_leaf() const
        {
                return children.empty() && state.choice_set.empty();
        }

        inline bool expand_fully() const
        {
                return state.choice_set.empty();
        }
};

class MontecarloTree {
public:
        MontecarloTree()
        {

        };

        inline double UCB1_explore(MontecarloNode *node)
        {
                double score;
                MontecarloNode *pa = node->parent;
//                if (node->N == 0) {
//                        score = 0x3f3f3f3f;
//                        return score;
//                }
                double C = 1 / sqrt(2);
                double exploit = (double) node->V / node->N;
                double explore = (double) 2 * C * sqrt(log(2 * pa->N) / node->N);
                score = (double) (exploit + explore);
                return score;
        }

        inline double UCB1_no_explore(MontecarloNode *node)
        {
                double score;
                MontecarloNode *pa = node->parent;
//                if (node->N == 0) {
//                        score = 0x3f3f3f3f;
//                        return score;
//                }
                double exploit = (double) node->V / node->N;
                double explore = 0.0;
                score = (double) (exploit + explore);
                return score;
        }

        MontecarloNode *montecarlo_expand(MontecarloNode *node)
        {

                unsigned int random_i = get_random(node->state.choice_set.size());
                P p = node->state.choice_set[random_i];
                node->state.choice_set.erase(node->state.choice_set.begin() + random_i);
                auto *new_node = new MontecarloNode(node);
                *new_node = *node;
                new_node->state.current_board[p.x][p.y] = node->state.who;
                new_node->parent = node;
                new_node->state.switch_player();
                new_node->state.get_all_choice();
                node->children.push_back(new_node);
                return new_node;
        }

        MontecarloNode *montecarlo_select(MontecarloNode *node)
        {
                double init_max = min_double;
                MontecarloNode *best = nullptr;
                for (auto &i : node->children) {
                        double ucb1 = UCB1_explore(i);
                        if (ucb1 > init_max) {
                                init_max = ucb1;
                                best = i;
                        }
                }
                return best;
        }

        void montecarlo_back_propagation(MontecarloNode *node, double v)
        {
                double value = v;
                while (node != nullptr) {
                        node->N++;
                        node->add_value(value);
                        value = -value;
                        node = node->parent;
                }
        }

        double montecarlo_simulation(MontecarloNode *node)
        {
                return node->state.default_policy();
        }

        MontecarloNode *tree_policy(MontecarloNode *node)
        {
                // if leaf node just return this
                if (node->is_leaf()) {
                        return node;
                }
                        // if can't be expanded ucb choosing
                else if (node->expand_fully()) {
                        //ucb with explore
                        MontecarloNode *new_node = montecarlo_select(node);
                        return tree_policy(new_node);
                }
                        // expand it
                else {
                        return montecarlo_expand(node);
                }

        }
};

P *nopass_catching(int(*someboard)[9], int Col, int side)
{
        int col;
        //other side
        if (side == -1) {
                col = Col;
        } else {
                col = -Col;
        }

        P *nopass = (P *) malloc(sizeof(*nopass));
        for (int x = 0; x < 9; x++) {
                for (int y = 0; y < 9; y++) {
                        //i ,j is this point <i,j>
                        if (someboard[x][y] != EMPTY) { continue; }
                        int xx1 = x - 1;
                        int yy1 = y + 1;
                        int xx2 = x - 1;
                        int yy2 = y - 1;
                        int xx3 = x - 2;
                        int yy3 = y;
                        if (inBorder(xx1, yy1) && someboard[xx1][yy1] == -col &&
                            inBorder(xx2, yy2) && someboard[xx2][yy2] == -col &&
                            inBorder(xx3, yy3) && someboard[xx3][yy3] == -col) {
                                if (MontecarloState::judge_some_board_Available(x, y, Col, someboard)) {
                                        nopass->x = x;
                                        nopass->y = y;
                                        return nopass;
                                }
                        }

                        xx1 = x;
                        yy1 = y - 2;
                        xx2 = x - 1;
                        yy2 = y - 1;
                        xx3 = x + 1;
                        yy3 = y - 1;

                        if (inBorder(xx1, yy1) && someboard[xx1][yy1] == -col &&
                            inBorder(xx2, yy2) && someboard[xx2][yy2] == -col &&
                            inBorder(xx3, yy3) && someboard[xx2][yy3] == -col) {
                                if (MontecarloState::judge_some_board_Available(x, y, Col, someboard)) {
                                        nopass->x = x;
                                        nopass->y = y;
                                        return nopass;
                                }
                        }

                        xx1 = x + 2;
                        yy1 = y;
                        xx2 = x + 1;
                        yy2 = y - 1;
                        xx3 = x + 1;
                        yy3 = y + 1;
                        if (inBorder(xx1, yy1) && someboard[xx1][yy1] == -col &&
                            inBorder(xx2, yy2) && someboard[xx2][yy2] == -col &&
                            inBorder(xx3, yy3) && someboard[xx2][yy3] == -col) {
                                if (MontecarloState::judge_some_board_Available(x, y, Col, someboard)) {
                                        nopass->x = x;
                                        nopass->y = y;
                                        return nopass;
                                }
                        }

                        xx1 = x - 1;
                        yy1 = y + 1;
                        xx2 = x + 1;
                        yy2 = y + 1;
                        xx3 = x;
                        yy3 = y + 2;
                        if (inBorder(xx1, yy1) && someboard[xx1][yy1] == -col &&
                            inBorder(xx2, yy2) && someboard[xx2][yy2] == -col &&
                            inBorder(xx3, yy3) && someboard[xx2][yy3] == -col) {
                                if (MontecarloState::judge_some_board_Available(x, y, Col, someboard)) {
                                        nopass->x = x;
                                        nopass->y = y;
                                        return nopass;
                                }
                        }

                }
        }
        return nullptr;
}
// n <=30
/*
  #
  ! #
  #
 */

int eval(direction d, int other_col)
{
        int score = 0;
        //important
        int imp = 0;
        //less important
        int limp = 0;
        switch (d) {

                case up: {
                        if (other_y <= 7 && board[other_x][other_y + 1] == EMPTY &&
                            MontecarloState::judge_some_board_Available(other_x, other_y + 1, -other_col, board)) {
                                score += 1;
                                if (other_x - 1 >= 0) {
                                        if (board[other_x - 1][other_y + 1] == other_col) {

                                                imp++;
                                        }
                                }
                                if (other_x <= 7) {
                                        if (board[other_x + 1][other_y + 1] == other_col) {
                                                imp++;
                                        }
                                }
                                for (auto &i : board) {
                                        if (i[other_y + 1] == other_col) {
                                                limp++;
                                        }
                                }
                                score += (100 * imp + limp);
                        }
                        break;
                }
                case down: {
                        if (other_y >= 1 && board[other_x][other_y - 1] == EMPTY &&
                            MontecarloState::judge_some_board_Available(other_x, other_y - 1, -other_col, board)) {
                                score += 1;
                                if (other_x - 1 >= 0) {
                                        if (board[other_x - 1][other_y - 1] == other_col) {
                                                imp++;
                                        }
                                }
                                if (other_x <= 7) {
                                        if (board[other_x + 1][other_y - 1] == other_col) {
                                                imp++;
                                        }
                                }
                                for (auto &i : board) {
                                        if (i[other_y - 1] == other_col) {
                                                limp++;
                                        }
                                }
                                score += (100 * imp + limp);
                        }
                        break;
                }

                case l: {
                        if (other_x >= 1 && board[other_x - 1][other_y] == EMPTY &&
                            MontecarloState::judge_some_board_Available(other_x - 1, other_y, -other_col, board)) {
                                score += 1;
                                if (other_y <= 7) {
                                        if (board[other_x - 1][other_y + 1] == other_col) {
                                                imp++;
                                        }
                                }
                                if (other_y >= 1) {
                                        if (board[other_x - 1][other_y - 1] == other_col) {
                                                imp++;
                                        }
                                }
                                for (int i = 0; i < 9; i++) {
                                        if (board[other_x - 1][i] == other_col) {
                                                limp++;
                                        }
                                }
                                score += (100 * imp + limp);
                        }
                        break;
                }
                case r:
                        if (other_x <= 7 && board[other_x + 1][other_y] == EMPTY &&
                            MontecarloState::judge_some_board_Available(other_x + 1, other_y, -other_col, board)) {
                                score += 1;
                                if (other_y <= 7) {
                                        if (board[other_x + 1][other_y + 1] == other_col) {
                                                imp++;
                                        }
                                }
                                if (other_y >= 1) {
                                        if (board[other_x + 1][other_y - 1] == other_col) {
                                                imp++;
                                        }
                                }
                                for (int i = 0; i < 9; i++) {
                                        if (board[other_x + 1][i] == other_col) {
                                                limp++;
                                        }
                                }
                                score += (100 * imp + limp);
                        }
                        break;

        }
        return score;

}

P *first_passes(int(*some_board)[9], int col)
{

        int up_score = eval(direction::up, -col);
        if (is_boundary(other_x, other_y + 1) &&
            MontecarloState::judge_some_board_Available(other_x, other_y + 1, col, some_board)) { up_score += 50; }
        int down_score = eval(direction::down, -col);
        if (is_boundary(other_x, other_y - 1) &&
            MontecarloState::judge_some_board_Available(other_x, other_y - 1, col, some_board)) { down_score += 50; }
        int l_score = eval(direction::l, -col);
        if (is_boundary(other_x - 1, other_y) &&
            MontecarloState::judge_some_board_Available(other_x - 1, other_y, col, some_board)) { l_score += 50; }
        int r_score = eval(direction::r, -col);
        if (is_boundary(other_x + 1, other_y) &&
            MontecarloState::judge_some_board_Available(other_x + 1, other_y, col, some_board)) { r_score += 50; }
        P *pos = new P(-1, -1);
        int max_score = max(max(up_score, down_score), max(l_score, r_score));
        if (max_score == 0) {
                return nullptr;
        }
        if (max_score == up_score) {
                pos->x = other_x;
                pos->y = other_y + 1;
                return pos;
        }
        if (max_score == down_score) {
                pos->x = other_x;
                pos->y = other_y - 1;
                return pos;
        }
        if (max_score == l_score) {
                pos->x = other_x - 1;
                pos->y = other_y;
                return pos;

        }
        if (max_score == r_score) {
                pos->x = other_x + 1;
                pos->y = other_y;
                return pos;
        }
        return nullptr;
}

//find the eye of the other side
P *eye_catching(int(*someboard)[9], int col)
{
        P *eye = (P *) malloc(sizeof(*eye));
        for (int x = 0; x < 9; x++) {
                for (int y = 0; y < 9; y++) {
                        //i ,j is this point <i,j>
                        if (someboard[x][y] != EMPTY) { continue; }

                        int xx1 = x - 1;
                        int yy1 = y;
                        int xx2 = x;
                        int yy2 = y + 1;
                        int xx3 = x;
                        int yy3 = y - 1;

                        if (inBorder(xx1, yy1) && someboard[xx1][yy1] == -col &&
                            inBorder(xx2, yy2) && someboard[xx2][yy2] == -col &&
                            inBorder(xx3, yy3) && someboard[xx3][yy3] == -col) {
                                if (MontecarloState::judge_some_board_Available(x, y, col, someboard)) {
                                        eye->x = x;
                                        eye->y = y;
                                        return eye;
                                }
                        }

                        xx1 = x - 1;
                        yy1 = y;
                        xx2 = x + 1;
                        yy2 = y;
                        xx3 = x;
                        yy3 = y - 1;
                        if (inBorder(xx1, yy1) && someboard[xx1][yy1] == -col &&
                            inBorder(xx2, yy2) && someboard[xx2][yy2] == -col &&
                            inBorder(xx3, yy3) && someboard[xx3][yy3] == -col) {
                                if (MontecarloState::judge_some_board_Available(x, y, col, someboard)) {
                                        eye->x = x;
                                        eye->y = y;
                                        return eye;
                                }
                        }

                        xx1 = x;
                        yy1 = y + 1;
                        xx2 = x;
                        yy2 = y - 1;
                        xx3 = x + 1;
                        yy3 = y;
                        if (inBorder(xx1, yy1) && someboard[xx1][yy1] == -col &&
                            inBorder(xx2, yy2) && someboard[xx2][yy2] == -col &&
                            inBorder(xx3, yy3) && someboard[xx3][yy3] == -col) {
                                if (MontecarloState::judge_some_board_Available(x, y, col, someboard)) {
                                        eye->x = x;
                                        eye->y = y;
                                        return eye;
                                }
                        }

                        xx1 = x - 1;
                        yy1 = y;
                        xx2 = x + 1;
                        yy2 = y;
                        xx3 = x;
                        yy3 = y + 1;
                        if (inBorder(xx1, yy1) && someboard[xx1][yy1] == -col &&
                            inBorder(xx2, yy2) && someboard[xx2][yy2] == -col &&
                            inBorder(xx3, yy3) && someboard[xx3][yy3] == -col) {
                                if (MontecarloState::judge_some_board_Available(x, y, col, someboard)) {
                                        eye->x = x;
                                        eye->y = y;
                                        return eye;
                                }
                        }

                }
        }

        return nullptr;

}

/*

 # ! #
 ! # !
 # ! #

 */
P *tp_catching(int (*someboard)[9], int other_col)
{
        P *tp = (P *) malloc(sizeof(*tp));
        for (int x = 0; x < 9; x++) {
                for (int y = 0; y < 9; y++) {
                        if (x >= 1 && y <= 7 && someboard[x - 1][y + 1] == other_col) {
                                if (MontecarloState::judge_some_board_Available(x - 1, y, -other_col, someboard)) {
                                        tp->x = x - 1;
                                        tp->y = y;
                                        return tp;
                                }
                                if (MontecarloState::judge_some_board_Available(x, y + 1, -other_col, someboard)) {
                                        tp->x = x;
                                        tp->y = y + 1;
                                        return tp;
                                }
                        }

                        if (x <= 7 && y <= 7 && someboard[x + 1][y + 1] == other_col) {
                                if (MontecarloState::judge_some_board_Available(x + 1, y, -other_col, someboard)) {
                                        tp->x = x + 1;
                                        tp->y = y;
                                        return tp;
                                }
                                if (MontecarloState::judge_some_board_Available(x, y + 1, -other_col, someboard)) {
                                        tp->x = x;
                                        tp->y = y + 1;
                                        return tp;
                                }
                        }

                        if (x >= 1 && y >= 1 && someboard[x - 1][y - 1] == other_col) {
                                if (MontecarloState::judge_some_board_Available(x - 1, y, -other_col, someboard)) {
                                        tp->x = x - 1;
                                        tp->y = y;
                                        return tp;
                                }
                                if (MontecarloState::judge_some_board_Available(x, y - 1, -other_col, someboard)) {
                                        tp->x = x;
                                        tp->y = y - 1;
                                        return tp;
                                }
                        }
                        if (x <= 7 && y >= 1 && someboard[x + 1][y - 1] == other_col) {
                                if (MontecarloState::judge_some_board_Available(x, y - 1, -other_col, someboard)) {
                                        tp->x = x;
                                        tp->y = y - 1;
                                        return tp;
                                }
                                if (MontecarloState::judge_some_board_Available(x + 1, y, -other_col, someboard)) {
                                        tp->x = x + 1;
                                        tp->y = y;
                                        return tp;
                                }
                        }
                }
        }
        return nullptr;
}

int main()
{
        string str;
        int x, y;
        // 读入JSON
        getline(cin, str);
        int get_clock_start = clock();
        //getline(cin, str);
        Json::Reader reader;
        Json::Value input;
        reader.parse(str, input);

        int player;
        int first = 0;
        auto *node = new MontecarloNode(nullptr);
        int col = input["requests"][first]["x"].asInt();

        int turnID = input["responses"].size();
        if (col == -1) {
                player = 1;
        } else {
                player = -1;
        }

        MontecarloState state(player, turnID);
        //第一步时turnID等于0，注意是responses的size，而不是request的size
        for (int i = 0; i < turnID; i++) {
                x = input["requests"][i]["x"].asInt(), y = input["requests"][i]["y"].asInt();
                //对方，注意此处是requests
                if (x != -1) board[x][y] = -player;
                x = input["responses"][i]["x"].asInt(), y = input["responses"][i]["y"].asInt();
                //我方，注意此处是responses
                if (x != -1) board[x][y] = player;
        }
        x = input["requests"][turnID]["x"].asInt(), y = input["requests"][turnID]["y"].asInt();
        other_x = x;
        other_y = y;
        //对方，注意此处是requests
        if (x != -1) {
                board[x][y] = -player;
                montecarlo_nodes++;
        }
        //此时board[][]里存储的就是当前棋盘的所有棋子信息,x和y存的是对方最近一步下的棋

        Json::Value ret;
        Json::Value action;

        //only the first play
        if (x == -1) {
                P choice = range[get_random(32)];
                action["x"] = choice.x;
                action["y"] = choice.y;
                ret["response"] = action;
                ret["debug"] = "start.";//调试信息可写在这里
                Json::FastWriter writer;
                cout << writer.write(ret) << endl;
                return 0;
        }

        P *eye = eye_catching(board, player);
        if (eye != nullptr) {
                action["x"] = eye->x;
                action["y"] = eye->y;
                ret["response"] = action;
                ret["debug"] = "<15 pass in eye";
                Json::FastWriter writer;
                cout << writer.write(ret) << endl;
                return 0;
        }
        if (turnID < 10) {
                P *eye_side = first_passes(board, player);
                if (eye_side != nullptr) {
                        action["x"] = eye_side->x;
                        action["y"] = eye_side->y;
                        ret["response"] = action;
                        ret["debug"] = "<15 pass stop eye";
                        Json::FastWriter writer;
                        cout << writer.write(ret) << endl;
                        return 0;
                }
                P *tp = tp_catching(board, -player);
                if (tp != nullptr) {
                        action["x"] = tp->x;
                        action["y"] = tp->y;
                        ret["response"] = action;
                        ret["debug"] = "<15 pass stop eye";
                        Json::FastWriter writer;
                        cout << writer.write(ret) << endl;
                        return 0;
                }
        }
/*---------------------------------------------------------------------------------------------------------*/
        state.load_board();
        state.get_all_choice();
        node->state = state;
        auto *T = new MontecarloTree();
        if (turnID >= 15) {
                time_allowed = (int) (0.98 * (double) CLOCKS_PER_SEC);
        }
        while (clock() - get_clock_start < time_allowed) {
                MontecarloNode *new_node = T->tree_policy(node);
                double value = T->montecarlo_simulation(new_node);
                T->montecarlo_back_propagation(new_node, value);

        }
/*---------------------------------------------------------------------------------------------------------*/
        //ucb with no-explore
        MontecarloNode *best = T->montecarlo_select(node);
        for (int i = 0; i < 9; i++)
                for (int j = 0; j < 9; j++)
                        if (board[i][j] != best->state.current_board[i][j]) {
                                action["x"] = i;
                                action["y"] = j;
                                break;
                        }
        ret["response"] = action;
        char debuginfo[4096];
/*----------------------------------------------------------------------------------------------------------*/
        sprintf(debuginfo, "current id:%d | player:%s |total node nums:%d ", turnID,
                (player == BLACK) ? ("myside") : ("otherside"), montecarlo_nodes);
/*----------------------------------------------------------------------------------------------------------*/
        ret["debug"] = debuginfo;//调试信息可写在这里
        Json::FastWriter writer;
        cout << writer.write(ret) << endl;
        return 0;
}