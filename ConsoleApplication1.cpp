#define _CRT_SECURE_NO_WARNINGS
#include <graphics.h>
#include <conio.h>
#include <vector>
#include <queue>
#include <stack>
#include <map>
#include <string>
#include <sstream>
#include <algorithm>
#include <functional>
#include <memory>
#include <windows.h>
#include <time.h>
#include <stdlib.h>
#include <cmath>
#include <chrono>
#include <set>
#include <iomanip>

using namespace std;
using namespace chrono;

// ==================== 前向声明 ====================
void OutText(int x, int y, const char* text, COLORREF color = RGB(240, 240, 240), int fontSize = 20);

// ==================== 颜色定义 ====================
COLORREF GetColorByIndex(int index) {
    static COLORREF colors[] = {
        RGB(240, 240, 240),   // 0: 空/背景
        RGB(255, 80, 80),     // 1: 红色
        RGB(80, 255, 80),     // 2: 绿色
        RGB(80, 120, 255),    // 3: 蓝色
        RGB(255, 255, 80),    // 4: 黄色
        RGB(220, 80, 255),    // 5: 紫色
        RGB(80, 255, 255),    // 6: 青色
        RGB(255, 180, 80),    // 7: 橙色
        RGB(200, 200, 200),   // 8: 灰色
        RGB(255, 120, 200),   // 9: 粉色
        RGB(120, 255, 200),   // 10: 薄荷绿
        RGB(200, 120, 255)    // 11: 淡紫色
    };

    if (index < 0 || index >= 12) return colors[0];
    return colors[index];
}

const char* GetColorName(int index) {
    static const char* names[] = {
        "空", "红", "绿", "蓝", "黄",
        "紫", "青", "橙", "灰", "粉",
        "薄荷", "淡紫"
    };
    if (index < 0 || index >= 12) return "未知";
    return names[index];
}

// ==================== 数据结构定义 ====================
struct Tube {
    vector<int> colors;  // 从底部到顶部的水颜色 (0=空)
    int capacity;        // 试管容量

    Tube(int cap = 4) : capacity(cap) {}

    bool isEmpty() const { return colors.empty(); }
    bool isFull() const { return (int)colors.size() >= capacity; }
    int size() const { return (int)colors.size(); }
    int freeSpace() const { return capacity - (int)colors.size(); }

    // 获取顶部颜色，空试管返回-1
    int topColor() const {
        if (colors.empty()) return -1;
        return colors.back();
    }

    // 获取顶部连续同色水的数量
    int topSegmentSize() const {
        if (colors.empty()) return 0;
        int topColor = colors.back();
        int count = 0;
        for (auto it = colors.rbegin(); it != colors.rend(); ++it) {
            if (*it == topColor) count++;
            else break;
        }
        return count;
    }

    // 是否可以倒入颜色为color的水
    bool canPourInto(int color) const {
        if (isFull()) return false;
        return isEmpty() || topColor() == color;
    }

    // 倒入指定颜色的水
    void pourIn(int color, int amount) {
        for (int i = 0; i < amount && !isFull(); i++) {
            colors.push_back(color);
        }
    }

    // 倒出指定数量的水
    void pourOut(int amount) {
        if (amount >= (int)colors.size()) {
            colors.clear();
        }
        else {
            colors.resize(colors.size() - amount);
        }
    }

    // 检查试管是否已完成（只有一种颜色或为空）
    bool isComplete() const {
        if (colors.empty()) return true;
        int firstColor = colors[0];
        for (int color : colors) {
            if (color != firstColor) return false;
        }
        return true;
    }

    // 检查是否超容（用于可视化）
    bool isOverCapacity() const {
        return (int)colors.size() > capacity;
    }
};

// 游戏状态
struct GameState {
    vector<Tube> tubes;
    string operation;
    GameState* parent;
    int gCost;  // 从起始状态到当前状态的代价
    int hCost;  // 启发式代价估计
    int moveFrom;  // 从哪个试管移动
    int moveTo;    // 移动到哪个试管
    int moveAmount; // 移动数量
    bool isInvalid; // 是否为无效状态（用于可视化）

    GameState() : parent(NULL), gCost(0), hCost(0), moveFrom(-1), moveTo(-1), moveAmount(0), isInvalid(false) {}

    GameState(const GameState& other) {
        tubes = other.tubes;
        operation = other.operation;
        parent = other.parent;
        gCost = other.gCost;
        hCost = other.hCost;
        moveFrom = other.moveFrom;
        moveTo = other.moveTo;
        moveAmount = other.moveAmount;
        isInvalid = other.isInvalid;
    }

    // 生成状态唯一键
    string getKey() const {
        string key;
        for (const auto& tube : tubes) {
            key += "[";
            for (int color : tube.colors) {
                key += to_string(color) + ",";
            }
            key += "];";
        }
        return key;
    }

    // 计算启发式代价 - 改进为可采纳启发函数
    int calculateHeuristic() const {
        int cost = 0;

        // 计算每个试管中颜色变化的次数
        for (const auto& tube : tubes) {
            if (tube.isEmpty()) continue;

            // 统计颜色变化的次数
            int colorChanges = 0;
            for (size_t i = 1; i < tube.colors.size(); i++) {
                if (tube.colors[i] != tube.colors[i - 1]) {
                    colorChanges++;
                }
            }

            // 每个颜色变化至少需要一次移动来修正
            cost += colorChanges;
        }

        // 保守估计，确保不会高估实际代价
        return cost / 2;
    }

    // 深度复制
    GameState deepCopy() const {
        GameState newState;
        newState.tubes = this->tubes;
        newState.operation = this->operation;
        newState.parent = this->parent;
        newState.gCost = this->gCost;
        newState.hCost = this->hCost;
        newState.moveFrom = this->moveFrom;
        newState.moveTo = this->moveTo;
        newState.moveAmount = this->moveAmount;
        newState.isInvalid = this->isInvalid;
        return newState;
    }

    // 获取移动描述
    string getMoveDescription() const {
        if (moveFrom == -1 || moveTo == -1) return "初始状态";

        char buffer[100];
        int fromTube = moveFrom + 1;
        int toTube = moveTo + 1;
        int color = tubes[moveFrom].topColor();
        sprintf(buffer, "从试管%d倒入试管%d (颜色: %s, 数量: %d)",
            fromTube, toTube, GetColorName(color), moveAmount);
        return string(buffer);
    }

    // 检查是否为无效状态（超容）
    bool hasOverCapacityTube() const {
        for (const auto& tube : tubes) {
            if (tube.isOverCapacity()) return true;
        }
        return false;
    }
};

// ==================== 参数输入框结构体 ====================
struct ParamInputBox {
    int x, y, width, height;
    string text;
    bool isActive;
    bool isNumber;
    int maxLength;
    string label;

    ParamInputBox(int _x, int _y, int _w, int _h, const string& _label, bool _isNumber = true, int _maxLen = 3)
        : x(_x), y(_y), width(_w), height(_h), label(_label), isActive(false), isNumber(_isNumber), maxLength(_maxLen) {
        text = "";
    }

    void draw() {
        // 绘制标签
        settextcolor(RGB(240, 240, 240));
        setbkmode(TRANSPARENT);
        OutText(x, y - 25, label.c_str(), RGB(240, 240, 240), 18);

        // 绘制输入框背景
        if (isActive) {
            setfillcolor(RGB(80, 100, 120));
        }
        else {
            setfillcolor(RGB(60, 70, 90));
        }
        solidrectangle(x, y, x + width, y + height);

        // 绘制边框
        if (isActive) {
            setlinecolor(RGB(100, 200, 255));
        }
        else {
            setlinecolor(RGB(100, 120, 140));
        }
        setlinestyle(PS_SOLID, 2);
        rectangle(x, y, x + width, y + height);

        // 绘制文本
        if (!text.empty()) {
            OutText(x + 5, y + 5, text.c_str(), RGB(240, 240, 240), 20);
        }
        else if (!isActive) {
            OutText(x + 5, y + 5, "点击输入", RGB(150, 150, 150), 18);
        }

        // 绘制光标（如果激活）
        if (isActive && ((GetTickCount() / 500) % 2 == 0)) {
            int textWidth = text.length() * 12;
            int cursorX = x + 5 + textWidth;
            setlinecolor(RGB(240, 240, 240));
            line(cursorX, y + 5, cursorX, y + height - 5);
        }
    }

    bool contains(int mx, int my) {
        return mx >= x && mx <= x + width && my >= y && my <= y + height;
    }

    void activate() {
        isActive = true;
    }

    void deactivate() {
        isActive = false;
    }

    void addChar(char ch) {
        if (text.length() < maxLength) {
            if (isNumber) {
                if (ch >= '0' && ch <= '9') {
                    text += ch;
                }
            }
            else {
                text += ch;
            }
        }
    }

    void backspace() {
        if (!text.empty()) {
            text.pop_back();
        }
    }

    int getIntValue() {
        if (text.empty()) return 0;
        return stoi(text);
    }

    void clear() {
        text.clear();
    }
};

// ==================== 全局变量 ====================
const int SCREEN_WIDTH = 1350;      // 调整屏幕宽度
const int SCREEN_HEIGHT = 950;      // 增加高度以适应新组件
const int TUBE_WIDTH = 70;          // 缩小瓶子宽度
const int TUBE_HEIGHT = 250;        // 缩小瓶子高度
const int INFO_PANEL_X = 1020;      // 调整信息面板位置
const int INFO_PANEL_WIDTH = 300;   // 信息面板宽度

// 调整水壶间距和布局
const int TUBE_HORIZONTAL_SPACING = 100;
const int TUBES_PER_ROW = 6;
const int TUBE_VERTICAL_SPACING = 100;
const int TUBE_START_X = 60;
const int TUBE_START_Y = 180;       // 下移以适应参数配置区

vector<GameState> solutionPath;
int currentStep = 0;
bool isSolving = false;
bool solutionFound = false;
bool noSolution = false;            // 无解标志
string noSolutionReason = "";       // 无解原因
char statusMessage[100] = "就绪 - 点击试管选择源/目标";
int totalStatesExplored = 0;
int maxStatesInMemory = 0;
int initialEmptyTubes = 0;          // 初始空瓶数
long long solvingTime = 0;          // 求解时间（毫秒）
string currentAlgorithm = "BFS";    // 当前算法

// 游戏交互相关
int selectedTube = -1;              // 当前选中的试管索引
vector<int> highlightedTubes;       // 可操作的高亮试管

// 算法性能统计
struct AlgorithmStats {
    int statesExplored;
    int maxMemory;
    long long solvingTime;
    int solutionLength;
    string algorithmName;
    bool hasSolution;
    string solutionStatus;
};

AlgorithmStats bfsStats, dfsStats, astarStats;

// 参数配置
int currentN = 6;  // 水壶数
int currentK = 4;  // 颜色数
int currentM = 4;  // 容量

// 输入框
vector<ParamInputBox> paramInputBoxes;
bool showNoSolutionWarning = false;
int activeInputBoxIndex = -1;  // 当前激活的输入框索引

// ==================== 辅助函数声明 ====================
GameState GenerateCustomLevel(int n, int k, int m);
vector<GameState> generateNextStates(const GameState& current, vector<GameState>& invalidStates);
bool isGoalState(const GameState& state);
bool BFS_Solve(const GameState& start);
bool DFS_Solve(const GameState& start);
bool AStar_Solve(const GameState& start);
void drawTube(int index, const Tube& tube, int x, int y, bool isSelected = false,
    bool isHighlighted = false, bool isInvalid = false, bool isGoal = false);
void drawInfoPanel();
void drawAlgorithmPanel();
void drawParameterPanel();
void drawNoSolutionWarning();
void drawCurrentState(const GameState& state);
int getTubeAtPosition(int x, int y);
bool isPointInButton(int x, int y, int btnX, int btnY, int width, int height);
void handleTubeClick(int tubeIndex);
void resetGame();
void clearSolution();
vector<string> extractMoveSequence(const vector<GameState>& path);
void printSolutionToConsole(const vector<GameState>& path, const string& algorithm);
void drawButton(int x, int y, int width, int height, const char* text, bool isSelected = false,
    COLORREF bgColor = RGB(70, 130, 180), COLORREF textColor = RGB(240, 240, 240),
    int fontSize = 18);
void drawTransferArrow(int fromX, int fromY, int toX, int toY, bool isValid = true,
    const string& reason = "", int amount = 0, int color = -1);

// ==================== 辅助函数定义 ====================
void OutText(int x, int y, const char* text, COLORREF color, int fontSize) {
    static wchar_t wbuffer[256];
    int len = MultiByteToWideChar(CP_ACP, 0, text, -1, NULL, 0);
    if (len > 0 && len < 256) {
        MultiByteToWideChar(CP_ACP, 0, text, -1, wbuffer, len);

        // 设置清晰字体
        LOGFONT f;
        gettextstyle(&f);
        f.lfHeight = fontSize;
        f.lfWeight = FW_NORMAL;
        f.lfQuality = PROOF_QUALITY;    // 高质量抗锯齿
        wcscpy(f.lfFaceName, L"微软雅黑");
        settextstyle(&f);

        settextcolor(color);
        setbkmode(TRANSPARENT);
        outtextxy(x, y, wbuffer);
    }
}

// 绘制按钮函数
void drawButton(int x, int y, int width, int height, const char* text, bool isSelected,
    COLORREF bgColor, COLORREF textColor, int fontSize) {

    // 绘制按钮背景
    setfillcolor(bgColor);
    solidrectangle(x, y, x + width, y + height);

    // 绘制按钮边框
    if (isSelected) {
        // 选中的按钮有更亮的边框
        setlinecolor(RGB(255, 255, 100));
        setlinestyle(PS_SOLID, 3);
        rectangle(x - 2, y - 2, x + width + 2, y + height + 2);

        // 内部高光
        setlinecolor(RGB(255, 255, 200));
        setlinestyle(PS_SOLID, 1);
        rectangle(x, y, x + width, y + height);
    }
    else {
        setlinecolor(RGB(150, 150, 170));
        setlinestyle(PS_SOLID, 2);
        rectangle(x, y, x + width, y + height);
    }

    // 计算文本居中位置
    int textWidth = strlen(text) * fontSize / 2;
    int textX = x + (width - textWidth) / 2;
    int textY = y + (height - fontSize) / 2;

    // 绘制按钮文字
    OutText(textX, textY, text, textColor, fontSize);
}

// 绘制转移箭头
void drawTransferArrow(int fromX, int fromY, int toX, int toY, bool isValid,
    const string& reason, int amount, int color) {

    if (isValid) {
        setlinecolor(RGB(100, 255, 100));  // 绿色合法转移
        setlinestyle(PS_SOLID, 3);
    }
    else {
        setlinecolor(RGB(255, 100, 100));  // 红色无效转移
        setlinestyle(PS_DASH, 1);
        LINESTYLE ls;
        ls.thickness = 2;
        ls.style = 0xFF00;  // 虚线样式
        setlinestyle(&ls);
    }

    // 绘制箭头线
    line(fromX, fromY, toX, toY);

    // 绘制箭头头部
    double angle = atan2(toY - fromY, toX - fromX);
    int arrowSize = 15;

    // 箭头点1
    int x1 = toX - arrowSize * cos(angle - 0.3);
    int y1 = toY - arrowSize * sin(angle - 0.3);
    // 箭头点2
    int x2 = toX - arrowSize * cos(angle + 0.3);
    int y2 = toY - arrowSize * sin(angle + 0.3);

    line(toX, toY, x1, y1);
    line(toX, toY, x2, y2);

    // 绘制转移信息（在中点）
    int midX = (fromX + toX) / 2;
    int midY = (fromY + toY) / 2;

    if (isValid && amount > 0 && color > 0) {
        char info[50];
        sprintf(info, "%d×%s", amount, GetColorName(color));
        OutText(midX - 20, midY - 20, info, RGB(200, 255, 200), 16);
    }
    else if (!isValid && !reason.empty()) {
        OutText(midX - 30, midY - 20, reason.c_str(), RGB(255, 200, 200), 14);
    }
}

// 生成自定义初始状态
GameState GenerateCustomLevel(int n, int k, int m) {
    GameState state;
    srand((unsigned int)time(NULL));

    // 预判无解：水壶数小于颜色数
    if (n < k) {
        noSolution = true;
        noSolutionReason = "无解：水壶数 (" + to_string(n) + ") < 颜色数 (" + to_string(k) + ")";
        showNoSolutionWarning = true;

        // 创建空状态用于显示
        for (int i = 0; i < n; i++) {
            Tube tube(m);
            state.tubes.push_back(tube);
        }

        state.operation = "初始状态（无解）";
        state.parent = NULL;
        state.gCost = 0;
        state.hCost = 0;
        state.moveFrom = -1;
        state.moveTo = -1;
        state.moveAmount = 0;
        state.isInvalid = true;

        return state;
    }

    // 重置无解标志
    noSolution = false;
    noSolutionReason = "";
    showNoSolutionWarning = false;

    // 创建颜色池
    vector<int> colorPool;
    for (int i = 1; i <= k; i++) {
        // 每种颜色添加m次
        for (int j = 0; j < m; j++) {
            colorPool.push_back(i);
        }
    }

    // 打乱颜色
    random_shuffle(colorPool.begin(), colorPool.end());

    // 创建试管并分配颜色
    int colorIndex = 0;
    initialEmptyTubes = 0;  // 重置空瓶计数

    for (int i = 0; i < n; i++) {
        Tube tube(m);

        // 前k个试管装满（如果n>k），或者全部装满（如果n==k）
        if (i < k) {
            for (int j = 0; j < m && colorIndex < (int)colorPool.size(); j++) {
                tube.colors.push_back(colorPool[colorIndex++]);
            }
        }
        else {
            initialEmptyTubes++;  // 统计空瓶
        }

        state.tubes.push_back(tube);
    }

    state.operation = "初始状态";
    if (n == k) {
        state.operation += " (紧约束场景)";
    }
    else if (n > k) {
        state.operation += " (有空余水壶)";
    }

    state.parent = NULL;
    state.gCost = 0;
    state.hCost = state.calculateHeuristic();
    state.moveFrom = -1;
    state.moveTo = -1;
    state.moveAmount = 0;
    state.isInvalid = false;

    return state;
}

// 生成下一状态，同时记录无效转移
vector<GameState> generateNextStates(const GameState& current, vector<GameState>& invalidStates) {
    vector<GameState> nextStates;
    int n = (int)current.tubes.size();

    for (int from = 0; from < n; from++) {
        if (current.tubes[from].isEmpty()) continue;

        int topColor = current.tubes[from].topColor();
        int segmentSize = current.tubes[from].topSegmentSize();

        for (int to = 0; to < n; to++) {
            if (from == to) continue;

            // 检查转移是否有效
            bool isValid = true;
            string invalidReason = "";

            // 检查目标试管是否能接收
            if (current.tubes[to].isFull()) {
                isValid = false;
                invalidReason = "超容";
            }
            else if (!current.tubes[to].isEmpty() && current.tubes[to].topColor() != topColor) {
                isValid = false;
                invalidReason = "不同色";
            }

            if (isValid) {
                int maxPour = min(segmentSize, current.tubes[to].freeSpace());
                if (maxPour == 0) continue;

                GameState next = current.deepCopy();
                next.tubes[from].pourOut(maxPour);
                next.tubes[to].pourIn(topColor, maxPour);

                // 记录移动信息
                next.moveFrom = from;
                next.moveTo = to;
                next.moveAmount = maxPour;

                char op[100];
                sprintf(op, "%d→%d (颜色%d, %d单位)", from + 1, to + 1, topColor, maxPour);
                next.operation = op;
                next.parent = new GameState(current);
                next.gCost = current.gCost + 1;
                next.hCost = next.calculateHeuristic();
                next.isInvalid = false;
                nextStates.push_back(next);
            }
            else {
                // 记录无效转移状态（用于可视化）
                GameState invalid = current.deepCopy();
                invalid.moveFrom = from;
                invalid.moveTo = to;
                invalid.moveAmount = 0;
                invalid.isInvalid = true;

                char op[100];
                sprintf(op, "无效: %d→%d (%s)", from + 1, to + 1, invalidReason.c_str());
                invalid.operation = op;
                invalidStates.push_back(invalid);
            }
        }
    }

    return nextStates;
}

// 检查是否为目标状态（符合新规则）
bool isGoalState(const GameState& state) {
    // 1. 检查每种颜色是否只出现在一个瓶子中
    map<int, int> colorToTubeMap; // 颜色 -> 瓶子索引
    for (int i = 0; i < (int)state.tubes.size(); i++) {
        const Tube& tube = state.tubes[i];
        if (tube.isEmpty()) continue; // 空瓶跳过

        // 检查瓶子内部是否只有一种颜色
        if (!tube.isComplete()) return false;

        int tubeColor = tube.colors[0];

        // 检查该颜色是否已出现在其他瓶子
        if (colorToTubeMap.find(tubeColor) != colorToTubeMap.end()) {
            return false; // 该颜色已出现在其他瓶
        }
        colorToTubeMap[tubeColor] = i;
    }

    // 2. 检查空瓶数量是否与初始状态一致
    int currentEmpty = 0;
    for (const auto& tube : state.tubes) {
        if (tube.isEmpty()) currentEmpty++;
    }

    return currentEmpty == initialEmptyTubes;
}

// 从路径中提取移动序列
vector<string> extractMoveSequence(const vector<GameState>& path) {
    vector<string> moves;

    // 从第二个状态开始（第一个是初始状态）
    for (size_t i = 1; i < path.size(); i++) {
        moves.push_back(path[i].getMoveDescription());
    }

    return moves;
}

// 打印解决方案到控制台
void printSolutionToConsole(const vector<GameState>& path, const string& algorithm) {
    if (path.empty()) return;

    printf("\n================ %s算法求解完成 ================\n", algorithm.c_str());
    printf("解决方案（共%d步）:\n", (int)path.size() - 1);
    printf("==============================================\n");

    for (size_t i = 1; i < path.size(); i++) {
        printf("第%2d步: %s\n", (int)i, path[i].getMoveDescription().c_str());
    }

    printf("==============================================\n");
    printf("初始状态:\n");
    for (size_t i = 0; i < path[0].tubes.size(); i++) {
        printf("  试管%d: ", (int)i + 1);
        if (path[0].tubes[i].isEmpty()) {
            printf("空");
        }
        else {
            for (int color : path[0].tubes[i].colors) {
                printf("%s ", GetColorName(color));
            }
        }
        printf("\n");
    }

    printf("\n最终状态:\n");
    for (size_t i = 0; i < path.back().tubes.size(); i++) {
        printf("  试管%d: ", (int)i + 1);
        if (path.back().tubes[i].isEmpty()) {
            printf("空");
        }
        else {
            for (int color : path.back().tubes[i].colors) {
                printf("%s ", GetColorName(color));
            }
        }
        printf("\n");
    }
    printf("==============================================\n\n");
}

// ==================== 算法实现 ====================
bool BFS_Solve(const GameState& start) {
    if (noSolution && start.isInvalid) return false;

    auto startTime = high_resolution_clock::now();

    queue<GameState*> q;
    map<string, bool> visited;

    GameState* startPtr = new GameState(start);
    startPtr->operation = "初始状态";
    startPtr->parent = NULL;
    startPtr->gCost = 0;
    startPtr->hCost = 0;
    startPtr->moveFrom = -1;
    startPtr->moveTo = -1;
    startPtr->moveAmount = 0;
    startPtr->isInvalid = false;

    q.push(startPtr);
    visited[startPtr->getKey()] = true;

    totalStatesExplored = 0;
    maxStatesInMemory = 0;
    GameState* goalState = NULL;

    while (!q.empty()) {
        int currentQueueSize = (int)q.size();
        maxStatesInMemory = max(maxStatesInMemory, currentQueueSize);

        GameState* current = q.front();
        q.pop();
        totalStatesExplored++;

        if (isGoalState(*current)) {
            goalState = current;
            break;
        }

        vector<GameState> invalidStates;
        vector<GameState> nextStates = generateNextStates(*current, invalidStates);
        for (size_t i = 0; i < nextStates.size(); i++) {
            string key = nextStates[i].getKey();
            if (visited.find(key) == visited.end()) {
                GameState* nextPtr = new GameState(nextStates[i]);
                nextPtr->parent = current;  // 设置父指针
                visited[key] = true;
                q.push(nextPtr);
            }
        }

        sprintf(statusMessage, "BFS搜索中... 已探索: %d", totalStatesExplored);

        if (totalStatesExplored % 100 == 0) {
            // 更新显示但不清除整个屏幕
            settextcolor(RGB(240, 240, 240));
            char progress[100];
            sprintf(progress, "BFS搜索中... 已探索状态: %d", totalStatesExplored);
            OutText(400, 100, progress, RGB(240, 240, 240), 24);
            FlushBatchDraw();
        }
    }

    if (goalState != NULL) {
        auto endTime = high_resolution_clock::now();
        solvingTime = duration_cast<milliseconds>(endTime - startTime).count();

        // 回溯构建路径
        solutionPath.clear();
        GameState* state = goalState;
        while (state != NULL) {
            solutionPath.insert(solutionPath.begin(), *state);
            state = state->parent;
        }

        // 打印解决方案到控制台
        printSolutionToConsole(solutionPath, "BFS");

        // 记录算法统计
        bfsStats.statesExplored = totalStatesExplored;
        bfsStats.maxMemory = maxStatesInMemory;
        bfsStats.solvingTime = solvingTime;
        bfsStats.solutionLength = (int)solutionPath.size() - 1;
        bfsStats.algorithmName = "BFS";
        bfsStats.hasSolution = true;
        bfsStats.solutionStatus = "有解";

        return true;
    }

    auto endTime = high_resolution_clock::now();
    solvingTime = duration_cast<milliseconds>(endTime - startTime).count();

    // 记录无解统计
    bfsStats.statesExplored = totalStatesExplored;
    bfsStats.maxMemory = maxStatesInMemory;
    bfsStats.solvingTime = solvingTime;
    bfsStats.solutionLength = 0;
    bfsStats.algorithmName = "BFS";
    bfsStats.hasSolution = false;
    bfsStats.solutionStatus = "无解";

    noSolution = true;
    noSolutionReason = "无解：搜索后未找到解决方案";
    showNoSolutionWarning = true;

    return false;
}

bool DFS_Solve(const GameState& start) {
    if (noSolution && start.isInvalid) return false;

    auto startTime = high_resolution_clock::now();

    stack<GameState*> s;
    map<string, bool> visited;

    GameState* startPtr = new GameState(start);
    startPtr->operation = "初始状态";
    startPtr->parent = NULL;
    startPtr->gCost = 0;
    startPtr->hCost = 0;
    startPtr->moveFrom = -1;
    startPtr->moveTo = -1;
    startPtr->moveAmount = 0;
    startPtr->isInvalid = false;

    s.push(startPtr);
    visited[startPtr->getKey()] = true;

    totalStatesExplored = 0;
    maxStatesInMemory = 0;
    GameState* goalState = NULL;
    int minSteps = INT_MAX;

    while (!s.empty()) {
        int currentStackSize = (int)s.size();
        maxStatesInMemory = max(maxStatesInMemory, currentStackSize);

        GameState* current = s.top();
        s.pop();
        totalStatesExplored++;

        if (isGoalState(*current)) {
            // DFS不一定找到最短路径，记录找到的第一个解
            if (goalState == NULL || current->gCost < minSteps) {
                goalState = current;
                minSteps = current->gCost;
            }
            continue;  // 继续搜索可能找到更短路径
        }

        vector<GameState> invalidStates;
        vector<GameState> nextStates = generateNextStates(*current, invalidStates);
        for (size_t i = 0; i < nextStates.size(); i++) {
            string key = nextStates[i].getKey();
            if (visited.find(key) == visited.end()) {
                GameState* nextPtr = new GameState(nextStates[i]);
                nextPtr->parent = current;
                visited[key] = true;
                s.push(nextPtr);
            }
        }

        sprintf(statusMessage, "DFS搜索中... 已探索: %d", totalStatesExplored);

        if (totalStatesExplored % 100 == 0) {
            settextcolor(RGB(240, 240, 240));
            char progress[100];
            sprintf(progress, "DFS搜索中... 已探索状态: %d", totalStatesExplored);
            OutText(400, 100, progress, RGB(240, 240, 240), 24);
            FlushBatchDraw();
        }
    }

    if (goalState != NULL) {
        auto endTime = high_resolution_clock::now();
        solvingTime = duration_cast<milliseconds>(endTime - startTime).count();

        // 回溯构建路径
        solutionPath.clear();
        GameState* state = goalState;
        while (state != NULL) {
            solutionPath.insert(solutionPath.begin(), *state);
            state = state->parent;
        }

        // 打印解决方案到控制台
        printSolutionToConsole(solutionPath, "DFS");

        // 记录算法统计
        dfsStats.statesExplored = totalStatesExplored;
        dfsStats.maxMemory = maxStatesInMemory;
        dfsStats.solvingTime = solvingTime;
        dfsStats.solutionLength = (int)solutionPath.size() - 1;
        dfsStats.algorithmName = "DFS";
        dfsStats.hasSolution = true;
        dfsStats.solutionStatus = "有解";

        return true;
    }

    auto endTime = high_resolution_clock::now();
    solvingTime = duration_cast<milliseconds>(endTime - startTime).count();

    // 记录无解统计
    dfsStats.statesExplored = totalStatesExplored;
    dfsStats.maxMemory = maxStatesInMemory;
    dfsStats.solvingTime = solvingTime;
    dfsStats.solutionLength = 0;
    dfsStats.algorithmName = "DFS";
    dfsStats.hasSolution = false;
    dfsStats.solutionStatus = "无解";

    noSolution = true;
    noSolutionReason = "无解：搜索后未找到解决方案";
    showNoSolutionWarning = true;

    return false;
}

// A*算法比较结构体
struct AStarCompare {
    bool operator()(const GameState* a, const GameState* b) const {
        // 首先比较f值，如果相同则比较h值
        int f1 = a->gCost + a->hCost;
        int f2 = b->gCost + b->hCost;
        if (f1 != f2) return f1 > f2;
        return a->hCost > b->hCost;  // 倾向于h值更小的状态
    }
};

bool AStar_Solve(const GameState& start) {
    if (noSolution && start.isInvalid) return false;

    auto startTime = high_resolution_clock::now();

    priority_queue<GameState*, vector<GameState*>, AStarCompare> pq;
    map<string, int> visited;  // 记录每个状态的最小gCost

    GameState* startPtr = new GameState(start);
    startPtr->operation = "初始状态";
    startPtr->parent = NULL;
    startPtr->gCost = 0;
    startPtr->hCost = startPtr->calculateHeuristic();
    startPtr->moveFrom = -1;
    startPtr->moveTo = -1;
    startPtr->moveAmount = 0;
    startPtr->isInvalid = false;

    pq.push(startPtr);
    visited[startPtr->getKey()] = startPtr->gCost;

    totalStatesExplored = 0;
    maxStatesInMemory = 0;
    GameState* goalState = NULL;

    while (!pq.empty()) {
        int currentQueueSize = (int)pq.size();
        maxStatesInMemory = max(maxStatesInMemory, currentQueueSize);

        GameState* current = pq.top();
        pq.pop();
        totalStatesExplored++;

        // 验证当前状态是否是最优路径上的（不是被更优路径取代的）
        if (visited[current->getKey()] < current->gCost) {
            continue;
        }

        if (isGoalState(*current)) {
            goalState = current;
            break;
        }

        vector<GameState> invalidStates;
        vector<GameState> nextStates = generateNextStates(*current, invalidStates);
        for (size_t i = 0; i < nextStates.size(); i++) {
            string key = nextStates[i].getKey();
            int newGCost = current->gCost + 1;

            if (visited.find(key) == visited.end() || newGCost < visited[key]) {
                GameState* nextPtr = new GameState(nextStates[i]);
                nextPtr->parent = current;
                nextPtr->gCost = newGCost;
                nextPtr->hCost = nextPtr->calculateHeuristic();

                visited[key] = newGCost;
                pq.push(nextPtr);
            }
        }

        sprintf(statusMessage, "A*搜索中... 已探索: %d", totalStatesExplored);

        if (totalStatesExplored % 100 == 0) {
            settextcolor(RGB(240, 240, 240));
            char progress[100];
            sprintf(progress, "A*搜索中... 已探索状态: %d", totalStatesExplored);
            OutText(400, 100, progress, RGB(240, 240, 240), 24);
            FlushBatchDraw();
        }
    }

    if (goalState != NULL) {
        auto endTime = high_resolution_clock::now();
        solvingTime = duration_cast<milliseconds>(endTime - startTime).count();

        // 回溯构建路径
        solutionPath.clear();
        GameState* state = goalState;
        while (state != NULL) {
            solutionPath.insert(solutionPath.begin(), *state);
            state = state->parent;
        }

        // 打印解决方案到控制台
        printSolutionToConsole(solutionPath, "A*");

        // 记录算法统计
        astarStats.statesExplored = totalStatesExplored;
        astarStats.maxMemory = maxStatesInMemory;
        astarStats.solvingTime = solvingTime;
        astarStats.solutionLength = (int)solutionPath.size() - 1;
        astarStats.algorithmName = "A*";
        astarStats.hasSolution = true;
        astarStats.solutionStatus = "有解";

        return true;
    }

    auto endTime = high_resolution_clock::now();
    solvingTime = duration_cast<milliseconds>(endTime - startTime).count();

    // 记录无解统计
    astarStats.statesExplored = totalStatesExplored;
    astarStats.maxMemory = maxStatesInMemory;
    astarStats.solvingTime = solvingTime;
    astarStats.solutionLength = 0;
    astarStats.algorithmName = "A*";
    astarStats.hasSolution = false;
    astarStats.solutionStatus = "无解";

    noSolution = true;
    noSolutionReason = "无解：搜索后未找到解决方案";
    showNoSolutionWarning = true;

    return false;
}

// ==================== 绘图函数 ====================
void drawTube(int index, const Tube& tube, int x, int y, bool isSelected,
    bool isHighlighted, bool isInvalid, bool isGoal) {
    // 计算行和列
    int row = index / TUBES_PER_ROW;
    int col = index % TUBES_PER_ROW;

    int tubeX = x + col * (TUBE_WIDTH + TUBE_HORIZONTAL_SPACING);
    int tubeY = y + row * (TUBE_HEIGHT + TUBE_VERTICAL_SPACING);

    // 绘制试管背景（透明效果）
    if (isGoal) {
        setfillcolor(RGB(100, 255, 100, 50));  // 目标状态绿色背景
    }
    else if (isInvalid) {
        setfillcolor(RGB(255, 100, 100, 50));  // 无效状态红色背景
    }
    else {
        setfillcolor(RGB(50, 50, 60, 100));
    }
    solidrectangle(tubeX, tubeY, tubeX + TUBE_WIDTH, tubeY + TUBE_HEIGHT);

    // 绘制试管边框
    if (isSelected) {
        setlinecolor(RGB(255, 255, 0));
        setlinestyle(PS_SOLID, 4);
    }
    else if (isHighlighted) {
        setlinecolor(RGB(100, 255, 100));
        setlinestyle(PS_SOLID, 3);
    }
    else if (isInvalid) {
        setlinecolor(RGB(255, 100, 100));  // 无效状态红色边框
        setlinestyle(PS_SOLID, 2);
    }
    else if (tube.isOverCapacity()) {
        setlinecolor(RGB(255, 50, 50));  // 超容红色边框
        setlinestyle(PS_SOLID, 3);
    }
    else {
        setlinecolor(RGB(150, 150, 170));
        setlinestyle(PS_SOLID, 2);
    }

    // 绘制试管轮廓
    rectangle(tubeX, tubeY, tubeX + TUBE_WIDTH, tubeY + TUBE_HEIGHT);

    // 绘制试管顶部弧形
    int neckWidth = TUBE_WIDTH * 0.7;
    int neckX = tubeX + (TUBE_WIDTH - neckWidth) / 2;
    line(neckX, tubeY, neckX, tubeY - 15);
    line(neckX + neckWidth, tubeY, neckX + neckWidth, tubeY - 15);
    arc(neckX, tubeY - 15, neckX + neckWidth, tubeY + 15, 3.14159, 6.28318);

    // 绘制试管内的水
    if (!tube.colors.empty()) {
        int waterUnitHeight = TUBE_HEIGHT / tube.capacity;

        for (int i = 0; i < (int)tube.colors.size(); i++) {
            int colorIdx = tube.colors[i];
            int waterY = tubeY + TUBE_HEIGHT - (i + 1) * waterUnitHeight;

            // 绘制水柱单元
            setfillcolor(GetColorByIndex(colorIdx));
            solidrectangle(tubeX + 2, waterY, tubeX + TUBE_WIDTH - 2,
                tubeY + TUBE_HEIGHT - i * waterUnitHeight);

            // 水柱单元之间的分割线（浅色）
            if (i > 0 && tube.colors[i] != tube.colors[i - 1]) {
                setlinecolor(RGB(255, 255, 255, 100));
                setlinestyle(PS_SOLID, 1);
                line(tubeX + 5, waterY, tubeX + TUBE_WIDTH - 5, waterY);
            }
        }

        // 绘制水面效果（最顶部）
        int topY = tubeY + TUBE_HEIGHT - tube.colors.size() * waterUnitHeight;
        setlinecolor(RGB(255, 255, 255, 150));
        setlinestyle(PS_SOLID, 2);
        line(tubeX + 5, topY, tubeX + TUBE_WIDTH - 5, topY);
    }

    // 绘制试管标签
    settextcolor(RGB(240, 240, 240));
    setbkmode(TRANSPARENT);
    char label[50];
    sprintf(label, "%d", index + 1);
    OutText(tubeX + TUBE_WIDTH / 2 - 10, tubeY + TUBE_HEIGHT + 10, label, RGB(240, 240, 240), 20);

    // 显示试管状态信息
    if (!tube.isEmpty()) {
        sprintf(label, "顶:%s", GetColorName(tube.topColor()));
        OutText(tubeX, tubeY + TUBE_HEIGHT + 35, label, RGB(200, 200, 200), 18);
    }

    // 显示容量信息
    sprintf(label, "%d/%d", tube.size(), tube.capacity);
    OutText(tubeX, tubeY - 35, label, RGB(180, 180, 200), 16);
}

void drawNoSolutionWarning() {
    if (!showNoSolutionWarning) return;

    // 绘制半透明背景
    setfillcolor(RGB(50, 0, 0, 200));
    solidrectangle(SCREEN_WIDTH / 2 - 250, SCREEN_HEIGHT / 2 - 100,
        SCREEN_WIDTH / 2 + 250, SCREEN_HEIGHT / 2 + 100);

    // 绘制边框
    setlinecolor(RGB(255, 50, 50));
    setlinestyle(PS_SOLID, 3);
    rectangle(SCREEN_WIDTH / 2 - 250, SCREEN_HEIGHT / 2 - 100,
        SCREEN_WIDTH / 2 + 250, SCREEN_HEIGHT / 2 + 100);

    // 绘制警告文字
    OutText(SCREEN_WIDTH / 2 - 230, SCREEN_HEIGHT / 2 - 70, "无解警告", RGB(255, 100, 100), 28);
    OutText(SCREEN_WIDTH / 2 - 230, SCREEN_HEIGHT / 2 - 30, noSolutionReason.c_str(), RGB(255, 200, 200), 22);

    if (noSolutionReason.find("水壶数") != string::npos) {
        OutText(SCREEN_WIDTH / 2 - 230, SCREEN_HEIGHT / 2 + 20,
            "请增加水壶数或减少颜色数", RGB(255, 255, 200), 20);
    }
    else {
        OutText(SCREEN_WIDTH / 2 - 230, SCREEN_HEIGHT / 2 + 20,
            "此参数组合无有效解决方案", RGB(255, 255, 200), 20);
    }
}

void drawInfoPanel() {
    // 信息面板背景
    setfillcolor(RGB(40, 45, 60, 220));
    solidrectangle(INFO_PANEL_X, 50, INFO_PANEL_X + INFO_PANEL_WIDTH, SCREEN_HEIGHT - 50);

    // 面板边框
    setlinecolor(RGB(80, 85, 100));
    setlinestyle(PS_SOLID, 2);
    rectangle(INFO_PANEL_X, 50, INFO_PANEL_X + INFO_PANEL_WIDTH, SCREEN_HEIGHT - 50);

    // 标题
    OutText(INFO_PANEL_X + 90, 70, "游戏控制", RGB(255, 215, 0), 26);

    int y = 120;
    OutText(INFO_PANEL_X + 20, y, statusMessage, RGB(240, 240, 240), 20);

    y += 45;
    char stepInfo[100];
    if (solutionFound) {
        sprintf(stepInfo, "当前步骤: %d / 总步: %d", currentStep, max(1, (int)solutionPath.size() - 1));
    }
    else {
        sprintf(stepInfo, "当前步骤: %d / 总步: 0", currentStep);
    }
    OutText(INFO_PANEL_X + 20, y, stepInfo, RGB(240, 240, 240), 20);

    if (solutionFound) {
        y += 35;
        char stateInfo[100];
        sprintf(stateInfo, "探索状态: %d", totalStatesExplored);
        OutText(INFO_PANEL_X + 20, y, stateInfo, RGB(200, 200, 240), 20);

        y += 35;
        char memInfo[100];
        sprintf(memInfo, "最大内存: %d", maxStatesInMemory);
        OutText(INFO_PANEL_X + 20, y, memInfo, RGB(200, 200, 240), 20);

        y += 35;
        char timeInfo[100];
        sprintf(timeInfo, "求解时间: %lld ms", solvingTime);
        OutText(INFO_PANEL_X + 20, y, timeInfo, RGB(200, 200, 240), 20);
    }

    // 绘制颜色图例
    y += 60;
    OutText(INFO_PANEL_X + 20, y, "颜色图例:", RGB(255, 255, 200), 22);
    y += 35;

    for (int i = 1; i <= min(6, currentK); i++) {
        int row = (i - 1) / 2;
        int col = (i - 1) % 2;

        int legendX = INFO_PANEL_X + 20 + col * 120;
        int legendY = y + row * 35;

        // 颜色方块
        setfillcolor(GetColorByIndex(i));
        solidrectangle(legendX, legendY, legendX + 25, legendY + 25);

        // 颜色名称
        char legendText[50];
        sprintf(legendText, "%s", GetColorName(i));
        OutText(legendX + 30, legendY, legendText, RGB(240, 240, 240), 18);
    }

    // 按钮区域
    y = SCREEN_HEIGHT - 420;
    int buttonWidth = 120;
    int buttonHeight = 40;
    int buttonSpacing = 18;

    // 求解按钮
    bool canSolve = !noSolution && !solutionPath.empty();
    drawButton(INFO_PANEL_X + 20, y, buttonWidth, buttonHeight,
        "自动求解", false,
        canSolve ? RGB(70, 130, 180) : RGB(80, 80, 100),
        canSolve ? RGB(240, 240, 240) : RGB(180, 180, 180), 18);

    // 上一步按钮
    bool canPrev = (currentStep > 0);
    drawButton(INFO_PANEL_X + INFO_PANEL_WIDTH - buttonWidth - 20, y, buttonWidth, buttonHeight,
        "上一步", false, canPrev ? RGB(70, 130, 180) : RGB(80, 80, 100),
        canPrev ? RGB(240, 240, 240) : RGB(180, 180, 180), 18);

    y += buttonHeight + buttonSpacing;

    // 重置按钮
    drawButton(INFO_PANEL_X + 20, y, buttonWidth, buttonHeight,
        "重置", false, RGB(70, 130, 180), RGB(240, 240, 240), 18);

    // 下一步按钮
    bool canNext = solutionFound && (currentStep < (int)solutionPath.size() - 1);
    drawButton(INFO_PANEL_X + INFO_PANEL_WIDTH - buttonWidth - 20, y, buttonWidth, buttonHeight,
        "下一步", false, canNext ? RGB(70, 130, 180) : RGB(80, 80, 100),
        canNext ? RGB(240, 240, 240) : RGB(180, 180, 180), 18);

    y += buttonHeight + buttonSpacing;

    // 新关卡按钮
    drawButton(INFO_PANEL_X + 20, y, buttonWidth, buttonHeight,
        "生成场景", false, RGB(180, 100, 70), RGB(240, 240, 240), 18);

    // 清除解按钮
    drawButton(INFO_PANEL_X + INFO_PANEL_WIDTH - buttonWidth - 20, y, buttonWidth, buttonHeight,
        "清除解", false, RGB(100, 180, 70), RGB(240, 240, 240), 18);

    // 算法选择区域
    y += buttonHeight + 45;
    OutText(INFO_PANEL_X + 20, y, "选择算法:", RGB(255, 255, 200), 22);
    y += 35;

    // BFS算法按钮
    COLORREF bfsBgColor = currentAlgorithm == "BFS" ? RGB(100, 170, 230) : RGB(70, 130, 180);
    COLORREF bfsTextColor = currentAlgorithm == "BFS" ? RGB(255, 255, 150) : RGB(240, 240, 240);
    drawButton(INFO_PANEL_X + 20, y, 80, 35, "BFS", currentAlgorithm == "BFS",
        bfsBgColor, bfsTextColor, 18);

    // DFS算法按钮
    COLORREF dfsBgColor = currentAlgorithm == "DFS" ? RGB(100, 170, 230) : RGB(70, 130, 180);
    COLORREF dfsTextColor = currentAlgorithm == "DFS" ? RGB(255, 255, 150) : RGB(240, 240, 240);
    drawButton(INFO_PANEL_X + 110, y, 80, 35, "DFS", currentAlgorithm == "DFS",
        dfsBgColor, dfsTextColor, 18);

    // A*算法按钮
    COLORREF astarBgColor = currentAlgorithm == "A*" ? RGB(100, 170, 230) : RGB(70, 130, 180);
    COLORREF astarTextColor = currentAlgorithm == "A*" ? RGB(255, 255, 150) : RGB(240, 240, 240);
    drawButton(INFO_PANEL_X + 200, y, 80, 35, "A*", currentAlgorithm == "A*",
        astarBgColor, astarTextColor, 18);

}

void drawParameterPanel() {
    int panelX = 20;
    int panelY = 20;
    int panelWidth = SCREEN_WIDTH - INFO_PANEL_WIDTH - 60;
    int panelHeight = 120;

    // 参数面板背景
    setfillcolor(RGB(40, 45, 60, 220));
    solidrectangle(panelX, panelY, panelX + panelWidth, panelY + panelHeight);

    // 面板边框
    setlinecolor(RGB(80, 85, 100));
    setlinestyle(PS_SOLID, 2);
    rectangle(panelX, panelY, panelX + panelWidth, panelY + panelHeight);

    // 标题
    OutText(panelX + 30, panelY + 50, "参数配置区", RGB(255, 215, 0), 24);

    // 绘制输入框
    for (size_t i = 0; i < paramInputBoxes.size(); i++) {
        paramInputBoxes[i].draw();
    }

    // 绘制当前场景信息
    int infoX = panelX + 550;
    OutText(infoX, panelY + 15, "当前场景:", RGB(255, 255, 200), 22);

    char sceneInfo[200];
    if (noSolution) {
        sprintf(sceneInfo, "无解场景: %s", noSolutionReason.c_str());
        OutText(infoX, panelY + 45, sceneInfo, RGB(255, 100, 100), 18);
    }
    else {
        if (currentN == currentK) {
            sprintf(sceneInfo, "紧约束场景 (水壶数 = 颜色数 = %d)", currentN);
            OutText(infoX, panelY + 45, sceneInfo, RGB(100, 255, 100), 18);
        }
        else if (currentN > currentK) {
            sprintf(sceneInfo, "松约束场景 (水壶数 %d > 颜色数 %d)", currentN, currentK);
            OutText(infoX, panelY + 45, sceneInfo, RGB(100, 200, 255), 18);
        }
        sprintf(sceneInfo, "容量: %d, 空水壶: %d", currentM, initialEmptyTubes);
        OutText(infoX, panelY + 70, sceneInfo, RGB(200, 200, 240), 18);
    }
}

void drawAlgorithmPanel() {
    int panelX = 20;
    int panelY = SCREEN_HEIGHT - 450;
    int panelWidth = SCREEN_WIDTH - INFO_PANEL_WIDTH - 60;

    // 算法统计面板背景
    setfillcolor(RGB(40, 45, 60, 200));
    solidrectangle(panelX, panelY, panelX + panelWidth, SCREEN_HEIGHT - 20);

    // 标题
    OutText(panelX + 10, panelY + 10, "算法性能对比", RGB(255, 215, 0), 24);

    // 表头
    int y = panelY + 45;
    OutText(panelX + 20, y, "算法", RGB(255, 255, 200), 20);
    OutText(panelX + 100, y, "状态数", RGB(255, 255, 200), 20);
    OutText(panelX + 200, y, "内存", RGB(255, 255, 200), 20);
    OutText(panelX + 300, y, "时间(ms)", RGB(255, 255, 200), 20);
    OutText(panelX + 420, y, "步数", RGB(255, 255, 200), 20);
    OutText(panelX + 500, y, "状态", RGB(255, 255, 200), 20);

    y += 35;

    // BFS统计
    OutText(panelX + 20, y, "BFS",
        currentAlgorithm == "BFS" ? RGB(255, 255, 150) : RGB(240, 240, 240), 20);
    if (bfsStats.statesExplored > 0) {
        char buf[100];
        sprintf(buf, "%d", bfsStats.statesExplored);
        OutText(panelX + 100, y, buf,
            currentAlgorithm == "BFS" ? RGB(255, 255, 150) : RGB(240, 240, 240), 20);
        sprintf(buf, "%d", bfsStats.maxMemory);
        OutText(panelX + 200, y, buf,
            currentAlgorithm == "BFS" ? RGB(255, 255, 150) : RGB(240, 240, 240), 20);
        sprintf(buf, "%lld", bfsStats.solvingTime);
        OutText(panelX + 300, y, buf,
            currentAlgorithm == "BFS" ? RGB(255, 255, 150) : RGB(240, 240, 240), 20);

        if (bfsStats.hasSolution) {
            sprintf(buf, "%d", bfsStats.solutionLength);
            OutText(panelX + 420, y, buf,
                currentAlgorithm == "BFS" ? RGB(255, 255, 150) : RGB(240, 240, 240), 20);
            OutText(panelX + 500, y, bfsStats.solutionStatus.c_str(),
                currentAlgorithm == "BFS" ? RGB(100, 255, 100) : RGB(100, 255, 100), 20);
        }
        else {
            OutText(panelX + 420, y, "-",
                currentAlgorithm == "BFS" ? RGB(255, 150, 150) : RGB(255, 150, 150), 20);
            OutText(panelX + 500, y, bfsStats.solutionStatus.c_str(),
                currentAlgorithm == "BFS" ? RGB(255, 100, 100) : RGB(255, 100, 100), 20);
        }
    }
    else {
        OutText(panelX + 100, y, "-", RGB(150, 150, 150), 20);
        OutText(panelX + 200, y, "-", RGB(150, 150, 150), 20);
        OutText(panelX + 300, y, "-", RGB(150, 150, 150), 20);
        OutText(panelX + 420, y, "-", RGB(150, 150, 150), 20);
        OutText(panelX + 500, y, "未运行", RGB(150, 150, 150), 20);
    }

    y += 35;

    // DFS统计
    OutText(panelX + 20, y, "DFS",
        currentAlgorithm == "DFS" ? RGB(255, 255, 150) : RGB(240, 240, 240), 20);
    if (dfsStats.statesExplored > 0) {
        char buf[100];
        sprintf(buf, "%d", dfsStats.statesExplored);
        OutText(panelX + 100, y, buf,
            currentAlgorithm == "DFS" ? RGB(255, 255, 150) : RGB(240, 240, 240), 20);
        sprintf(buf, "%d", dfsStats.maxMemory);
        OutText(panelX + 200, y, buf,
            currentAlgorithm == "DFS" ? RGB(255, 255, 150) : RGB(240, 240, 240), 20);
        sprintf(buf, "%lld", dfsStats.solvingTime);
        OutText(panelX + 300, y, buf,
            currentAlgorithm == "DFS" ? RGB(255, 255, 150) : RGB(240, 240, 240), 20);

        if (dfsStats.hasSolution) {
            sprintf(buf, "%d", dfsStats.solutionLength);
            OutText(panelX + 420, y, buf,
                currentAlgorithm == "DFS" ? RGB(255, 255, 150) : RGB(240, 240, 240), 20);
            OutText(panelX + 500, y, dfsStats.solutionStatus.c_str(),
                currentAlgorithm == "DFS" ? RGB(100, 255, 100) : RGB(100, 255, 100), 20);
        }
        else {
            OutText(panelX + 420, y, "-",
                currentAlgorithm == "DFS" ? RGB(255, 150, 150) : RGB(255, 150, 150), 20);
            OutText(panelX + 500, y, dfsStats.solutionStatus.c_str(),
                currentAlgorithm == "DFS" ? RGB(255, 100, 100) : RGB(255, 100, 100), 20);
        }
    }
    else {
        OutText(panelX + 100, y, "-", RGB(150, 150, 150), 20);
        OutText(panelX + 200, y, "-", RGB(150, 150, 150), 20);
        OutText(panelX + 300, y, "-", RGB(150, 150, 150), 20);
        OutText(panelX + 420, y, "-", RGB(150, 150, 150), 20);
        OutText(panelX + 500, y, "未运行", RGB(150, 150, 150), 20);
    }

    y += 35;

    // A*统计
    OutText(panelX + 20, y, "A*",
        currentAlgorithm == "A*" ? RGB(255, 255, 150) : RGB(240, 240, 240), 20);
    if (astarStats.statesExplored > 0) {
        char buf[100];
        sprintf(buf, "%d", astarStats.statesExplored);
        OutText(panelX + 100, y, buf,
            currentAlgorithm == "A*" ? RGB(255, 255, 150) : RGB(240, 240, 240), 20);
        sprintf(buf, "%d", astarStats.maxMemory);
        OutText(panelX + 200, y, buf,
            currentAlgorithm == "A*" ? RGB(255, 255, 150) : RGB(240, 240, 240), 20);
        sprintf(buf, "%lld", astarStats.solvingTime);
        OutText(panelX + 300, y, buf,
            currentAlgorithm == "A*" ? RGB(255, 255, 150) : RGB(240, 240, 240), 20);

        if (astarStats.hasSolution) {
            sprintf(buf, "%d", astarStats.solutionLength);
            OutText(panelX + 420, y, buf,
                currentAlgorithm == "A*" ? RGB(255, 255, 150) : RGB(240, 240, 240), 20);
            OutText(panelX + 500, y, astarStats.solutionStatus.c_str(),
                currentAlgorithm == "A*" ? RGB(100, 255, 100) : RGB(100, 255, 100), 20);
        }
        else {
            OutText(panelX + 420, y, "-",
                currentAlgorithm == "A*" ? RGB(255, 150, 150) : RGB(255, 150, 150), 20);
            OutText(panelX + 500, y, astarStats.solutionStatus.c_str(),
                currentAlgorithm == "A*" ? RGB(255, 100, 100) : RGB(255, 100, 100), 20);
        }
    }
    else {
        OutText(panelX + 100, y, "-", RGB(150, 150, 150), 20);
        OutText(panelX + 200, y, "-", RGB(150, 150, 150), 20);
        OutText(panelX + 300, y, "-", RGB(150, 150, 150), 20);
        OutText(panelX + 420, y, "-", RGB(150, 150, 150), 20);
        OutText(panelX + 500, y, "未运行", RGB(150, 150, 150), 20);
    }

    // 当前算法提示
    y += 45;
    char currentAlgMsg[100];
    sprintf(currentAlgMsg, "当前算法: %s", currentAlgorithm.c_str());

    // 根据当前算法改变提示颜色
    COLORREF currentAlgColor;
    if (currentAlgorithm == "BFS") currentAlgColor = RGB(150, 200, 255);
    else if (currentAlgorithm == "DFS") currentAlgColor = RGB(150, 255, 200);
    else currentAlgColor = RGB(255, 200, 150);

    OutText(panelX + 20, y, currentAlgMsg, currentAlgColor, 24);

    // 游戏目标说明
    y += 40;
    OutText(panelX + 20, y, "游戏目标: 所有相同颜色液体集中到同一瓶子中", RGB(200, 240, 200), 20);
    OutText(panelX + 20, y + 30, "空瓶子数量保持和起始状态一致", RGB(200, 240, 200), 20);
}

void drawCurrentState(const GameState& state) {
    cleardevice();

    // 绘制背景
    setfillcolor(RGB(25, 30, 40));
    solidrectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    // 绘制标题
    OutText(350, 20, "水排序拼图算法可视化系统", RGB(255, 215, 0), 32);
    OutText(400, 60, "支持自定义参数与无解场景分析", RGB(200, 220, 255), 24);

    // 绘制所有试管
    if (!solutionPath.empty() && currentStep < (int)solutionPath.size()) {
        const GameState& currentState = solutionPath[currentStep];

        for (int i = 0; i < (int)currentState.tubes.size(); i++) {
            bool isSelected = (i == selectedTube);
            bool isHighlighted = find(highlightedTubes.begin(), highlightedTubes.end(), i) != highlightedTubes.end();
            bool isGoalTube = solutionFound && currentStep == (int)solutionPath.size() - 1 &&
                currentState.tubes[i].isComplete() && !currentState.tubes[i].isEmpty();
            drawTube(i, currentState.tubes[i], TUBE_START_X, TUBE_START_Y,
                isSelected, isHighlighted, currentState.isInvalid, isGoalTube);
        }

        // 显示当前步骤
        char stepText[200];
        if (currentStep == 0) {
            sprintf(stepText, "初始状态");
        }
        else {
            sprintf(stepText, "步骤 %d/%d: %s",
                currentStep,
                max(1, (int)solutionPath.size() - 1),
                currentState.operation.c_str());
        }
        OutText(100, 145, stepText, RGB(255, 255, 180), 22);

        // 如果当前步骤有移动，绘制转移箭头
        if (currentStep > 0 && currentState.moveFrom != -1 && currentState.moveTo != -1) {
            // 计算试管中心位置
            int fromRow = currentState.moveFrom / TUBES_PER_ROW;
            int fromCol = currentState.moveFrom % TUBES_PER_ROW;
            int toRow = currentState.moveTo / TUBES_PER_ROW;
            int toCol = currentState.moveTo % TUBES_PER_ROW;

            int fromX = TUBE_START_X + fromCol * (TUBE_WIDTH + TUBE_HORIZONTAL_SPACING) + TUBE_WIDTH / 2;
            int fromY = TUBE_START_Y + fromRow * (TUBE_HEIGHT + TUBE_VERTICAL_SPACING) - 20;
            int toX = TUBE_START_X + toCol * (TUBE_WIDTH + TUBE_HORIZONTAL_SPACING) + TUBE_WIDTH / 2;
            int toY = TUBE_START_Y + toRow * (TUBE_HEIGHT + TUBE_VERTICAL_SPACING) - 20;

            drawTransferArrow(fromX, fromY, toX, toY, true, "",
                currentState.moveAmount, currentState.tubes[currentState.moveFrom].topColor());
        }
    }

    // 绘制各个面板
    drawParameterPanel();
    drawInfoPanel();
    drawAlgorithmPanel();

    // 绘制无解警告（如果有）
    drawNoSolutionWarning();

    // 绘制底部状态栏
    setfillcolor(RGB(50, 55, 70));
    solidrectangle(0, SCREEN_HEIGHT - 25, SCREEN_WIDTH, SCREEN_HEIGHT);
    OutText(400, SCREEN_HEIGHT - 22, "《算法设计与分析》课程项目 - 水排序问题可视化求解系统", RGB(200, 220, 240), 18);
}

int getTubeAtPosition(int x, int y) {
    if (solutionPath.empty() || currentStep >= (int)solutionPath.size()) return -1;

    for (int i = 0; i < (int)solutionPath[currentStep].tubes.size(); i++) {
        int row = i / TUBES_PER_ROW;
        int col = i % TUBES_PER_ROW;

        int currentX = TUBE_START_X + col * (TUBE_WIDTH + TUBE_HORIZONTAL_SPACING);
        int currentY = TUBE_START_Y + row * (TUBE_HEIGHT + TUBE_VERTICAL_SPACING);

        // 扩大点击区域，包括标签区域
        if (x >= currentX - 10 && x <= currentX + TUBE_WIDTH + 10 &&
            y >= currentY - 10 && y <= currentY + TUBE_HEIGHT + 50) {
            return i;
        }
    }
    return -1;
}

bool isPointInButton(int x, int y, int btnX, int btnY, int width, int height) {
    return x >= btnX && x <= btnX + width && y >= btnY && y <= btnY + height;
}

void handleTubeClick(int tubeIndex) {
    if (noSolution || tubeIndex < 0 || tubeIndex >= (int)solutionPath[currentStep].tubes.size()) return;

    const Tube& clickedTube = solutionPath[currentStep].tubes[tubeIndex];

    if (selectedTube == -1) {
        // 选择源试管
        if (!clickedTube.isEmpty()) {
            selectedTube = tubeIndex;
            sprintf(statusMessage, "已选择试管 %d", tubeIndex + 1);

            // 计算可倒入的目标试管
            highlightedTubes.clear();
            int topColor = clickedTube.topColor();
            for (int i = 0; i < (int)solutionPath[currentStep].tubes.size(); i++) {
                if (i != selectedTube && solutionPath[currentStep].tubes[i].canPourInto(topColor)) {
                    highlightedTubes.push_back(i);
                }
            }
        }
    }
    else {
        // 选择目标试管
        if (tubeIndex != selectedTube) {
            int topColor = solutionPath[currentStep].tubes[selectedTube].topColor();
            if (solutionPath[currentStep].tubes[tubeIndex].canPourInto(topColor)) {
                // 执行倒水操作
                GameState newState = solutionPath[currentStep].deepCopy();

                int from = selectedTube;
                int to = tubeIndex;
                int segmentSize = newState.tubes[from].topSegmentSize();
                int maxPour = min(segmentSize, newState.tubes[to].freeSpace());

                newState.tubes[from].pourOut(maxPour);
                newState.tubes[to].pourIn(topColor, maxPour);

                // 记录移动信息
                newState.moveFrom = from;
                newState.moveTo = to;
                newState.moveAmount = maxPour;

                char op[100];
                sprintf(op, "手动: %d→%d (颜色%d, %d单位)", from + 1, to + 1, topColor, maxPour);
                newState.operation = op;
                newState.parent = NULL;

                // 添加到路径
                solutionPath.insert(solutionPath.begin() + currentStep + 1, newState);
                currentStep++;

                sprintf(statusMessage, "从 %d 倒入 %d", from + 1, to + 1);

                // 检查是否胜利
                if (isGoalState(newState)) {
                    sprintf(statusMessage, "胜利! 关卡完成!");
                    solutionFound = true;
                }
            }
        }

        // 清除选择
        selectedTube = -1;
        highlightedTubes.clear();
    }
}

void resetGame() {
    currentStep = 0;
    selectedTube = -1;
    highlightedTubes.clear();
    strcpy(statusMessage, "已重置到初始状态");
}

void clearSolution() {
    solutionFound = false;
    solutionPath.resize(1);  // 只保留初始状态
    currentStep = 0;
    selectedTube = -1;
    highlightedTubes.clear();
    noSolution = false;
    noSolutionReason = "";
    showNoSolutionWarning = false;
    strcpy(statusMessage, "已清除求解结果");
}

// ==================== 主函数 ====================
int main() {
    // 分配控制台窗口用于输出
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    freopen("CONIN$", "r", stdin);

    srand((unsigned int)time(NULL));

    // 初始化图形窗口
    initgraph(SCREEN_WIDTH, SCREEN_HEIGHT);
    setbkcolor(RGB(25, 30, 40));
    cleardevice();

    // 启用批量绘图（减少闪烁）
    BeginBatchDraw();

    // 初始化输入框
    paramInputBoxes.clear();
    paramInputBoxes.push_back(ParamInputBox(230, 60, 80, 35, "水壶数(n)", true, 2));
    paramInputBoxes.push_back(ParamInputBox(340, 60, 80, 35, "颜色数(k)", true, 2));
    paramInputBoxes.push_back(ParamInputBox(450, 60, 80, 35, "容量(m)", true, 2));

    // 设置初始值
    paramInputBoxes[0].text = "4";
    paramInputBoxes[1].text = "4";
    paramInputBoxes[2].text = "4";

    // 生成初始关卡
    currentN = 4;
    currentK = 4;
    currentM = 4;
    GameState initialState = GenerateCustomLevel(currentN, currentK, currentM);
    solutionPath.push_back(initialState);

    // 初始化算法统计
    bfsStats = { 0, 0, 0, 0, "BFS", false, "未运行" };
    dfsStats = { 0, 0, 0, 0, "DFS", false, "未运行" };
    astarStats = { 0, 0, 0, 0, "A*", false, "未运行" };

    // 绘制初始界面
    drawCurrentState(initialState);
    FlushBatchDraw();

    printf("==============================================\n");
    printf("      水排序拼图算法可视化系统\n");
    printf("==============================================\n");
    printf("功能说明：\n");
    printf("1. 在参数配置区输入水壶数、颜色数、容量\n");
    printf("2. 点击'生成场景'按钮创建自定义场景\n");
    printf("3. 选择算法后点击'自动求解'\n");
    printf("4. 支持无解场景提示与可视化\n");
    printf("5. 支持动态转移演示和步骤回溯\n");
    printf("==============================================\n\n");
    printf("当前参数：水壶数=%d, 颜色数=%d, 容量=%d\n", currentN, currentK, currentM);

    // 主循环
    while (true) {
        // 处理鼠标输入
        if (MouseHit()) {
            MOUSEMSG msg = GetMouseMsg();

            if (msg.uMsg == WM_LBUTTONDOWN) {
                // 首先检查是否点击了输入框
                bool inputBoxClicked = false;
                for (size_t i = 0; i < paramInputBoxes.size(); i++) {
                    if (paramInputBoxes[i].contains(msg.x, msg.y)) {
                        // 取消之前激活的输入框
                        if (activeInputBoxIndex != -1 && activeInputBoxIndex < (int)paramInputBoxes.size()) {
                            paramInputBoxes[activeInputBoxIndex].deactivate();
                        }
                        // 激活新的输入框
                        paramInputBoxes[i].activate();
                        activeInputBoxIndex = (int)i;
                        inputBoxClicked = true;
                        break;
                    }
                }

                // 如果点击了非输入框区域，取消所有输入框的激活状态
                if (!inputBoxClicked) {
                    for (size_t i = 0; i < paramInputBoxes.size(); i++) {
                        paramInputBoxes[i].deactivate();
                    }
                    activeInputBoxIndex = -1;
                }

                // 如果输入框被点击，更新显示后继续
                if (inputBoxClicked) {
                    drawCurrentState(solutionPath[currentStep]);
                    FlushBatchDraw();
                    continue;
                }

                // 检查是否点击了试管
                int clickedTube = getTubeAtPosition(msg.x, msg.y);
                if (clickedTube != -1) {
                    handleTubeClick(clickedTube);
                    drawCurrentState(solutionPath[currentStep]);
                    FlushBatchDraw();
                    continue;
                }

                // 检查算法按钮点击
                int algorithmButtonY = SCREEN_HEIGHT - 420 + 3 * (40 + 18) + 45;

                // BFS算法按钮
                if (isPointInButton(msg.x, msg.y, INFO_PANEL_X + 20, algorithmButtonY, 80, 35)) {
                    currentAlgorithm = "BFS";
                    drawCurrentState(solutionPath[currentStep]);
                    FlushBatchDraw();
                    continue;
                }
                // DFS算法按钮
                else if (isPointInButton(msg.x, msg.y, INFO_PANEL_X + 110, algorithmButtonY, 80, 35)) {
                    currentAlgorithm = "DFS";
                    drawCurrentState(solutionPath[currentStep]);
                    FlushBatchDraw();
                    continue;
                }
                // A*算法按钮
                else if (isPointInButton(msg.x, msg.y, INFO_PANEL_X + 200, algorithmButtonY, 80, 35)) {
                    currentAlgorithm = "A*";
                    drawCurrentState(solutionPath[currentStep]);
                    FlushBatchDraw();
                    continue;
                }

                // 检查其他按钮点击
                int buttonWidth = 120;
                int buttonHeight = 40;
                int buttonSpacing = 18;
                int buttonY = SCREEN_HEIGHT - 420;

                // 自动求解按钮
                if (isPointInButton(msg.x, msg.y, INFO_PANEL_X + 20, buttonY, buttonWidth, buttonHeight)) {
                    if (!isSolving && !noSolution) {
                        isSolving = true;
                        sprintf(statusMessage, "%s算法求解中...", currentAlgorithm.c_str());
                        drawCurrentState(solutionPath[currentStep]);
                        FlushBatchDraw();

                        bool success = false;
                        if (currentAlgorithm == "BFS") {
                            success = BFS_Solve(solutionPath[currentStep]);
                        }
                        else if (currentAlgorithm == "DFS") {
                            success = DFS_Solve(solutionPath[currentStep]);
                        }
                        else if (currentAlgorithm == "A*") {
                            success = AStar_Solve(solutionPath[currentStep]);
                        }

                        if (success) {
                            sprintf(statusMessage, "%s算法求解完成! 步数: %d",
                                currentAlgorithm.c_str(), (int)solutionPath.size() - 1);
                            currentStep = 0;
                            solutionFound = true;

                            // 在控制台显示统计信息
                            printf("\n算法统计信息:\n");
                            printf("  算法: %s\n", currentAlgorithm.c_str());
                            printf("  探索状态数: %d\n", totalStatesExplored);
                            printf("  最大内存状态: %d\n", maxStatesInMemory);
                            printf("  求解时间: %lld ms\n", solvingTime);
                            printf("  解决方案步数: %d\n", (int)solutionPath.size() - 1);
                        }
                        else {
                            printf("\n%s算法未找到解决方案\n", currentAlgorithm.c_str());
                            printf("  原因: %s\n", noSolutionReason.c_str());
                            printf("  探索状态数: %d\n", totalStatesExplored);
                            printf("  求解时间: %lld ms\n", solvingTime);
                        }
                        isSolving = false;
                        selectedTube = -1;
                        highlightedTubes.clear();

                        drawCurrentState(solutionPath[currentStep]);
                        FlushBatchDraw();
                    }
                }
                // 上一步按钮
                else if (isPointInButton(msg.x, msg.y, INFO_PANEL_X + INFO_PANEL_WIDTH - buttonWidth - 20,
                    buttonY, buttonWidth, buttonHeight)) {
                    if (currentStep > 0) {
                        currentStep--;
                        selectedTube = -1;
                        highlightedTubes.clear();
                        sprintf(statusMessage, "回退到上一步");
                        drawCurrentState(solutionPath[currentStep]);
                        FlushBatchDraw();
                    }
                }
                // 重置按钮
                else if (isPointInButton(msg.x, msg.y, INFO_PANEL_X + 20,
                    buttonY + buttonHeight + buttonSpacing, buttonWidth, buttonHeight)) {
                    resetGame();
                    drawCurrentState(solutionPath[currentStep]);
                    FlushBatchDraw();
                }
                // 下一步按钮
                else if (isPointInButton(msg.x, msg.y, INFO_PANEL_X + INFO_PANEL_WIDTH - buttonWidth - 20,
                    buttonY + buttonHeight + buttonSpacing, buttonWidth, buttonHeight)) {
                    if (solutionFound && currentStep < (int)solutionPath.size() - 1) {
                        currentStep++;
                        selectedTube = -1;
                        highlightedTubes.clear();
                        drawCurrentState(solutionPath[currentStep]);
                        FlushBatchDraw();
                    }
                }
                // 生成场景按钮
                else if (isPointInButton(msg.x, msg.y, INFO_PANEL_X + 20,
                    buttonY + 2 * (buttonHeight + buttonSpacing), buttonWidth, buttonHeight)) {
                    // 获取输入框的值
                    currentN = paramInputBoxes[0].getIntValue();
                    currentK = paramInputBoxes[1].getIntValue();
                    currentM = paramInputBoxes[2].getIntValue();

                    // 验证输入
                    if (currentN <= 0) currentN = 1;
                    if (currentK <= 0) currentK = 1;
                    if (currentM <= 0) currentM = 1;

                    // 限制最大值
                    if (currentN > 12) currentN = 12;
                    if (currentK > 12) currentK = 12;
                    if (currentM > 10) currentM = 10;

                    // 更新输入框显示
                    paramInputBoxes[0].text = to_string(currentN);
                    paramInputBoxes[1].text = to_string(currentK);
                    paramInputBoxes[2].text = to_string(currentM);

                    // 生成新场景
                    clearSolution();
                    GameState newState = GenerateCustomLevel(currentN, currentK, currentM);
                    solutionPath.clear();
                    solutionPath.push_back(newState);
                    sprintf(statusMessage, "新场景已生成: n=%d, k=%d, m=%d", currentN, currentK, currentM);

                    printf("\n================ 新场景已生成 ================\n");
                    printf("参数: 水壶数=%d, 颜色数=%d, 容量=%d\n", currentN, currentK, currentM);
                    if (noSolution) {
                        printf("状态: %s\n", noSolutionReason.c_str());
                    }
                    else if (currentN == currentK) {
                        printf("状态: 紧约束场景 (无多余空水壶)\n");
                    }
                    else {
                        printf("状态: 松约束场景 (空水壶数: %d)\n", initialEmptyTubes);
                    }

                    drawCurrentState(solutionPath[currentStep]);
                    FlushBatchDraw();
                }
                // 清除解按钮
                else if (isPointInButton(msg.x, msg.y, INFO_PANEL_X + INFO_PANEL_WIDTH - buttonWidth - 20,
                    buttonY + 2 * (buttonHeight + buttonSpacing), buttonWidth, buttonHeight)) {
                    clearSolution();
                    drawCurrentState(solutionPath[currentStep]);
                    FlushBatchDraw();
                }
            }
        }

        // 处理键盘输入
        if (_kbhit()) {
            int key = _getch();
            bool needRedraw = false;

            // 检查是否有激活的输入框
            if (activeInputBoxIndex != -1 && activeInputBoxIndex < (int)paramInputBoxes.size()) {
                if (key == 13) { // Enter键 - 确认输入
                    paramInputBoxes[activeInputBoxIndex].deactivate();
                    activeInputBoxIndex = -1;
                }
                else if (key == 27) { // ESC键 - 取消输入
                    paramInputBoxes[activeInputBoxIndex].deactivate();
                    activeInputBoxIndex = -1;
                }
                else if (key == 8) { // Backspace键
                    paramInputBoxes[activeInputBoxIndex].backspace();
                    needRedraw = true;
                }
                else if (key >= 32 && key <= 126) { // 可打印字符
                    paramInputBoxes[activeInputBoxIndex].addChar((char)key);
                    needRedraw = true;
                }
            }
            else {
                // 没有激活的输入框，处理其他键盘命令
                if (key == 27) { // ESC键退出
                    break;
                }
                else if (key == 75 || key == 'a' || key == 'A') { // 左箭头/A键: 上一步
                    if (currentStep > 0) {
                        currentStep--;
                        selectedTube = -1;
                        highlightedTubes.clear();
                        needRedraw = true;
                    }
                }
                else if (key == 77 || key == 'd' || key == 'D') { // 右箭头/D键: 下一步
                    if (solutionFound && currentStep < (int)solutionPath.size() - 1) {
                        currentStep++;
                        selectedTube = -1;
                        highlightedTubes.clear();
                        needRedraw = true;
                    }
                }
                else if (key == ' ') { // 空格键: 自动求解
                    if (!isSolving && !noSolution) {
                        isSolving = true;
                        sprintf(statusMessage, "%s算法求解中...", currentAlgorithm.c_str());
                        drawCurrentState(solutionPath[currentStep]);
                        FlushBatchDraw();

                        bool success = false;
                        if (currentAlgorithm == "BFS") {
                            success = BFS_Solve(solutionPath[currentStep]);
                        }
                        else if (currentAlgorithm == "DFS") {
                            success = DFS_Solve(solutionPath[currentStep]);
                        }
                        else if (currentAlgorithm == "A*") {
                            success = AStar_Solve(solutionPath[currentStep]);
                        }

                        if (success) {
                            sprintf(statusMessage, "%s算法求解完成! 步数: %d",
                                currentAlgorithm.c_str(), (int)solutionPath.size() - 1);
                            currentStep = 0;
                            solutionFound = true;

                            // 在控制台显示统计信息
                            printf("\n算法统计信息:\n");
                            printf("  算法: %s\n", currentAlgorithm.c_str());
                            printf("  探索状态数: %d\n", totalStatesExplored);
                            printf("  最大内存状态: %d\n", maxStatesInMemory);
                            printf("  求解时间: %lld ms\n", solvingTime);
                            printf("  解决方案步数: %d\n", (int)solutionPath.size() - 1);
                        }
                        else {
                            printf("\n%s算法未找到解决方案\n", currentAlgorithm.c_str());
                            printf("  原因: %s\n", noSolutionReason.c_str());
                            printf("  探索状态数: %d\n", totalStatesExplored);
                            printf("  求解时间: %lld ms\n", solvingTime);
                        }
                        isSolving = false;
                        selectedTube = -1;
                        highlightedTubes.clear();
                        needRedraw = true;
                    }
                }
                else if (key == 'r' || key == 'R') { // R键: 重置
                    resetGame();
                    needRedraw = true;
                }
                else if (key == 'n' || key == 'N') { // N键: 生成场景
                    // 获取输入框的值
                    currentN = paramInputBoxes[0].getIntValue();
                    currentK = paramInputBoxes[1].getIntValue();
                    currentM = paramInputBoxes[2].getIntValue();

                    // 验证输入
                    if (currentN <= 0) currentN = 1;
                    if (currentK <= 0) currentK = 1;
                    if (currentM <= 0) currentM = 1;

                    // 更新输入框显示
                    paramInputBoxes[0].text = to_string(currentN);
                    paramInputBoxes[1].text = to_string(currentK);
                    paramInputBoxes[2].text = to_string(currentM);

                    // 生成新场景
                    clearSolution();
                    GameState newState = GenerateCustomLevel(currentN, currentK, currentM);
                    solutionPath.clear();
                    solutionPath.push_back(newState);
                    sprintf(statusMessage, "新场景已生成: n=%d, k=%d, m=%d", currentN, currentK, currentM);

                    printf("\n================ 新场景已生成 ================\n");
                    printf("参数: 水壶数=%d, 颜色数=%d, 容量=%d\n", currentN, currentK, currentM);
                    if (noSolution) {
                        printf("状态: %s\n", noSolutionReason.c_str());
                    }
                    needRedraw = true;
                }
                else if (key == 'c' || key == 'C') { // C键: 清除解
                    clearSolution();
                    needRedraw = true;
                }
                else if (key == '1') { // 1键: 选择BFS算法
                    currentAlgorithm = "BFS";
                    needRedraw = true;
                }
                else if (key == '2') { // 2键: 选择DFS算法
                    currentAlgorithm = "DFS";
                    needRedraw = true;
                }
                else if (key == '3') { // 3键: 选择A*算法
                    currentAlgorithm = "A*";
                    needRedraw = true;
                }
            }

            if (needRedraw) {
                drawCurrentState(solutionPath[currentStep]);
                FlushBatchDraw();
            }
        }

        // 短暂延时
        Sleep(10);
    }

    EndBatchDraw();
    closegraph();

    // 关闭控制台
    FreeConsole();
    return 0;
}