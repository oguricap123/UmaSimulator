//测试训练属性值算法
#include <iostream>
#include <random>
#include <sstream>
#include <cassert>
#include <thread>
#include <atomic>
#include "../Game/Game.h"
#include "../NeuralNet/Evaluator.h"
#include "../Search/Search.h"
#include "../External/termcolor.hpp"
using namespace std;

const bool handWrittenEvaluationTest = false;
const int threadNum = 20;
const int threadNumInner = 1;
const int searchN = handWrittenEvaluationTest ? 1 : 1024;


const int totalGames = handWrittenEvaluationTest ? 60000 : 10000000;
const int gamesEveryThread = totalGames / threadNum;


std::atomic<double> totalScore = 0;
std::atomic<double> totalScoreSqr = 0;

std::atomic<int> bestScore = 0;
std::atomic<int> n = 0;



void worker()
{
  random_device rd;
  auto rand = mt19937_64(rd());

  int umaId = 4;//我自己的号
  int cards[6] = { 1,2,3,4,5,6 };

  //int umaId = 3;//二之宫
  //int cards[6] = { 1,2,14,10,11,15 };

  int zhongmaBlue[5] = { 18,0,0,0,0 };
  int zhongmaBonus[6] = { 20,0,40,0,20,200 };

  Search search;
  vector<Evaluator> evaluators;
  for (int i = 0; i < threadNumInner; i++)
    evaluators.push_back(Evaluator(NULL, 128));

  for (int gamenum = 0; gamenum < gamesEveryThread; gamenum++)
  {

    Game game;
    game.newGame(rand, false, umaId, cards, zhongmaBlue, zhongmaBonus);

    while(!game.isEnd())
    {
      ModelOutputPolicyV1 policy;
      if (handWrittenEvaluationTest) {
        policy = Evaluator::handWrittenPolicy(game);
      }
      else {
        search.runSearch(game, evaluators.data(), searchN, TOTAL_TURN, 27000, threadNumInner);
        policy = search.extractPolicyFromSearchResults(1);
      }
      Search::runOneTurnUsingPolicy(rand, game, policy, true);
    }
    //cout << termcolor::red << "育成结束！" << termcolor::reset << endl;
    int score = game.finalScore();
    n += 1;
    totalScore += score;
    totalScoreSqr += score * score;

    int bestScoreOld = bestScore;
    while (score > bestScoreOld && !bestScore.compare_exchange_weak(bestScoreOld, score)) {
      // 如果val大于old_max，并且max_val的值还是old_max，那么就将max_val的值更新为val
      // 如果max_val的值已经被其他线程更新，那么就不做任何事情，并且old_max会被设置为max_val的新值
      // 然后我们再次进行比较和交换操作，直到成功为止
    }


    //game.print();
    if (!handWrittenEvaluationTest)
    {
      game.printFinalStats();
      cout << n << "局，搜索量=" << searchN << "，平均分" << totalScore / n << "，标准差" << sqrt(totalScoreSqr / n - totalScore * totalScore / n / n) << "，最高分" << bestScore << endl;
    }
  }

}

void main_test5()
{
  std::vector<std::thread> threads;
  for (int i = 0; i < threadNum; ++i) {
    threads.push_back(std::thread(worker));
  }
  for (auto& thread : threads) {
    thread.join();
  }

  cout << n << "局，搜索量=" << searchN << "，平均分" << totalScore / n << "，标准差" << sqrt(totalScoreSqr / n - totalScore * totalScore / n / n) << "，最高分" << bestScore << endl;

}