#define GRAPH_ALGOS_NO_MAIN
#include "../src/graph_algos.cpp"

#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <vector>

namespace {

/*
  Функция для запуска набора комманд

  - Параметры:
  -- const std::vector<std::string>& lines - вектор строк, комманды из которых
  будут обрабатываться

  - Возвращаемое значение:
  -- std::string - весь текст, что напечатал экземпляр класса Graph
*/
std::string RunCommands(const std::vector<std::string> &lines) {
  Graph graph{};

  std::ostringstream captured_output;
  std::streambuf *original_cout_buf = std::cout.rdbuf(captured_output.rdbuf());

  for (const std::string &line : lines) {
    if (line.empty()) {
      continue;
    }

    std::stringstream string_stream(line);
    graph.CommandProcessing(string_stream);
  }

  std::cout.rdbuf(original_cout_buf);
  return captured_output.str();
}

/*
  Функция, которая разбивает вывод на строки для более удобных построчных
  сравнений там, где порядок внутри блока не гарантирован (например, вывод
  Дейкстры идёт в порядке std::map)

  - Параметры:
  -- const std::string& text - текст, который будет разбиваться на строки

  - Возвращаемое значение:
  -- std::vector<std::string> - вектор из строк, полученных разбиением текста
*/
std::vector<std::string> SplitLines(const std::string &text) {
  std::vector<std::string> result;
  std::stringstream stream(text);
  std::string line;

  while (std::getline(stream, line)) {
    result.push_back(line);
  }

  return result;
}

} // namespace

/* Тесты базового функционала графа */

/* Тест добавления ребра с отсутствующей исходной вершиной */
TEST(GraphBasics, AddEdgeWithMissingSourceNode) {
  std::string output = RunCommands({"NODE B", "EDGE A B 5"});

  EXPECT_EQ(output, "Unknown node A\n");
}

/* Тест добавления ребра с отсутствующей целевой вершиной */
TEST(GraphBasics, AddEdgeWithMissingTargetNode) {
  std::string output = RunCommands({"NODE A", "EDGE A B 5"});

  EXPECT_EQ(output, "Unknown node B\n");
}

/* Тест добавления ребра с обеими отсутствующими вершинами */
TEST(GraphBasics, AddEdgeWithBothNodesMissing) {
  std::string output = RunCommands({"EDGE A B 5"});

  EXPECT_EQ(output, "Unknown nodes A B\n");
}

/* Тест повторного добавления ноды в граф */
TEST(GraphBasics, AddDuplicateNodeIsIgnoredSilently) {
  std::string output = RunCommands({"NODE A", "NODE A", "RPO_NUMBERING A"});

  EXPECT_EQ(output, "A\n");
}

/*
  Тест удаления существующей ноды и последующего добавления инцидентного ей
  ребра
*/
TEST(GraphBasics, RemoveExistingNode) {
  std::string output = RunCommands(
      {"NODE A", "NODE B", "EDGE A B 5", "REMOVE NODE B", "EDGE A B 5"});

  EXPECT_EQ(output, "Unknown node B\n");
}

/* Тест удаления несуществующей ноды */
TEST(GraphBasics, RemoveUnknownNode) {
  std::string output = RunCommands({"REMOVE NODE Z"});

  EXPECT_EQ(output, "Unknown node Z\n");
}

/* Тест удаления ребра с несуществующим концом */
TEST(GraphBasics, RemoveEdgeWithMissingNodes) {
  std::string output = RunCommands({"NODE A", "REMOVE EDGE A B"});

  EXPECT_EQ(output, "Unknown node B\n");
}

/* Тест удаления пути между вершинами для проверки корректности алгоритма
 * Дейкстры */
TEST(GraphBasics, RemoveEdgeThenRepathIsUnreachable) {
  std::string output = RunCommands(
      {"NODE A", "NODE B", "EDGE A B 5", "REMOVE EDGE A B", "DIJKSTRA A"});

  EXPECT_EQ(output, "");
}

/* Тест корректности удаления ноды: вместе с ней должны удалиться все
 * инцидентные ей рёбра */
TEST(GraphBasics, RemovingNodeAlsoRemovesIncidentEdges) {
  std::string output =
      RunCommands({"NODE A", "NODE B", "NODE C", "EDGE A B 1", "EDGE B C 1",
                   "REMOVE NODE B", "DIJKSTRA A"});

  EXPECT_EQ(output, "");
}

/*
  Тесты корректности работы RPONumbering
*/

/* Тест ошибки при запуске метода от несуществующей стартовой вершины */
TEST(RpoNumbering, UnknownStartNode) {
  std::string output = RunCommands({"RPO_NUMBERING A"});

  EXPECT_EQ(output, "Unknown node A\n");
}

/* Тест запуска алгоритма на графе из одного узла */
TEST(RpoNumbering, SingleNodeNoEdges) {
  std::string output = RunCommands({"NODE A", "RPO_NUMBERING A"});

  EXPECT_EQ(output, "A\n");
}

/* Тест запуска алгоритма на линейной цепочке(бамбуке) */
TEST(RpoNumbering, LinearChainOrder) {
  std::string output = RunCommands({"NODE A", "NODE B", "NODE C", "EDGE A B 1",
                                    "EDGE B C 1", "RPO_NUMBERING A"});

  EXPECT_EQ(output, "A B C\n");
}

/*
  Тест, проверяющий отсутсвие лишних ' ' в выводе(они же trailing space)
  Было сделано на всякий случай
*/
TEST(RpoNumbering, NoTrailingSpace) {
  std::string output =
      RunCommands({"NODE A", "NODE B", "EDGE A B 1", "RPO_NUMBERING A"});

  EXPECT_EQ(output, "A B\n");
  ASSERT_FALSE(output.empty());
  EXPECT_NE(output[output.size() - 2], ' ');
}

/* Проверка на обнаружение петель в графе */
TEST(RpoNumbering, DetectsSelfLoop) {
  std::string output = RunCommands({"NODE A", "EDGE A A 1", "RPO_NUMBERING A"});

  EXPECT_EQ(output, "Found loop A->A\nA\n");
}

/* Тест корректного обнаружения цикла в графе */
TEST(RpoNumbering, DetectsCycleAndReportsCorrectDirection) {
  std::string output =
      RunCommands({"NODE A", "NODE B", "NODE C", "EDGE A B 1", "EDGE B C 1",
                   "EDGE C A 1", "RPO_NUMBERING A"});

  std::vector<std::string> lines = SplitLines(output);

  ASSERT_EQ(lines.size(), 2u);
  EXPECT_EQ(lines[0], "Found loop C->A");
  EXPECT_EQ(lines[1], "A B C");
}

/* Тест непосещения вершин, недостижимых из стартового узла */
TEST(RpoNumbering, UnreachableNodesAreNotVisited) {
  std::string output = RunCommands(
      {"NODE A", "NODE B", "NODE C", "EDGE A B 1", "RPO_NUMBERING A"});

  EXPECT_EQ(output, "A B\n");
}

/*
  Тесты корректности работы алгоритма Дейкстры(а точнее метода Dijkstra класса
  Graph)
*/

/* Тест запуска алгоритма от несуществующей вершины */
TEST(Dijkstra, UnknownStartNode) {
  std::string output = RunCommands({"DIJKSTRA A"});

  EXPECT_EQ(output, "Unknown node A\n");
}

/* Тест примера из условия задачи */
TEST(Dijkstra, ExampleFromSpec) {
  std::string output =
      RunCommands({"NODE A", "NODE B", "NODE C", "NODE D", "EDGE A B 10",
                   "EDGE B C 10", "EDGE C D 10", "DIJKSTRA A"});

  EXPECT_EQ(output, "B 10\nC 20\nD 30\n");
}

/* Тест отсутствия стартовой ноды в выводе алгоритма */
TEST(Dijkstra, StartNodeIsNotPrintedInOwnResult) {
  std::string output =
      RunCommands({"NODE A", "NODE B", "EDGE A B 5", "DIJKSTRA A"});

  EXPECT_EQ(output, "B 5\n");
}

/* Тест для проверки минимальности выводимых расстояний */
TEST(Dijkstra, PicksShortestOfMultiplePaths) {
  std::string output =
      RunCommands({"NODE A", "NODE B", "NODE C", "EDGE A B 1", "EDGE B C 1",
                   "EDGE A C 100", "DIJKSTRA A"});

  EXPECT_EQ(output, "B 1\nC 2\n");
}

/*
  Тест отсутствия вершин, недостижимых из стартового узла, в выводе алгоритма
*/
TEST(Dijkstra, UnreachableNodesAreOmitted) {
  std::string output = RunCommands({"NODE A", "NODE B", "DIJKSTRA A"});

  EXPECT_EQ(output, "");
}

/*
  Тест корректной обработки вершины, достижимой из стартовой ноды по ребру
  нулевого веса
*/
TEST(Dijkstra, ZeroWeightEdgeIsHandledCorrectly) {
  std::string output =
      RunCommands({"NODE A", "NODE B", "EDGE A B 0", "DIJKSTRA A"});

  EXPECT_EQ(output, "B 0\n");
}

/*
  Тест для проверки корректности работы алгоритма Эдмондса-Карпа(через него
  получаем величину MAXFLOW между двумя нодами)
*/

/* Тест запуска алгоритма с несуществующим стоком */
TEST(MaxFlow, UnknownNodes) {
  std::string output = RunCommands({"NODE A", "MAXFLOW A G"});

  EXPECT_EQ(output, "Unknown node G\n");
}

/* Тест запуска алгоритма на сети из одной ноды */
TEST(MaxFlow, StartEqualsEndReturnsZero) {
  std::string output = RunCommands({"NODE A", "MAXFLOW A A"});

  EXPECT_EQ(output, "0\n");
}

/* Тест запуска алгоритма на сети из двух нод без пути между ними */
TEST(MaxFlow, NoPathReturnsZero) {
  std::string output = RunCommands({"NODE A", "NODE B", "MAXFLOW A B"});

  EXPECT_EQ(output, "0\n");
}

/*
  Тест запуска алгоритма на сети из двух нод с единственным ребром между ними
*/
TEST(MaxFlow, SingleEdgeCapacity) {
  std::string output =
      RunCommands({"NODE A", "NODE B", "EDGE A B 5", "MAXFLOW A B"});

  EXPECT_EQ(output, "5\n");
}

/* Тест корректной работы алгоритма на бамбуке */
TEST(MaxFlow, BottleneckOnLinearChain) {
  std::string output =
      RunCommands({"NODE A", "NODE B", "NODE C", "NODE D", "EDGE A B 10",
                   "EDGE B C 3", "EDGE C D 10", "MAXFLOW A D"});

  EXPECT_EQ(output, "3\n");
}

/* Тест алгоритма при наличии параллельных путей между источником и стоком */
TEST(MaxFlow, ParallelPathsSumUp) {
  std::string output =
      RunCommands({"NODE A", "NODE B", "NODE C", "NODE D", "EDGE A B 5",
                   "EDGE B D 5", "EDGE A C 5", "EDGE C D 5", "MAXFLOW A D"});

  EXPECT_EQ(output, "10\n");
}

/* Тест классического примера из курса АиСД */
TEST(MaxFlow, ClassicDiamondWithSharedMiddleEdge) {
  std::string output = RunCommands(
      {"NODE A", "NODE B", "NODE C", "NODE D", "EDGE A B 1000", "EDGE A C 1000",
       "EDGE B D 1000", "EDGE C D 1000", "EDGE B C 1", "MAXFLOW A D"});

  EXPECT_EQ(output, "2000\n");
}

/* Тест проверки использования обратного ребра в сети */
TEST(MaxFlow, UsesResidualBackwardEdgeToFindOptimalFlow) {
  std::string output = RunCommands({"NODE A", "NODE B", "NODE C", "NODE D",
                                    "EDGE A B 1", "EDGE A C 1", "EDGE C B 1",
                                    "EDGE B D 1", "EDGE C D 1", "MAXFLOW A D"});

  EXPECT_EQ(output, "2\n");
}

/* Тесты корректности работы алгоритма Тарьяна */

/* Тест работы алгоритма при невалидности стартовой вершины */
TEST(Tarjan, UnknownStartNode) {
  std::string output = RunCommands({"TARJAN A"});

  EXPECT_EQ(output, "Unknown node A\n");
}

/* Тест работы алгоритма на примере из условия задачи */
TEST(Tarjan, ExampleFromSpec) {
  std::string output = RunCommands({"NODE A", "NODE B", "NODE C", "EDGE A B 1",
                                    "EDGE B C 1", "EDGE C A 1", "TARJAN A"});

  EXPECT_EQ(output, "A B C\n");
}

/* Тест отсутствия в выводе КСС мощности 1(то есть таких, что их |V| = 1) */
TEST(Tarjan, SingleNodeSccsAreFilteredOut) {
  std::string output = RunCommands(
      {"NODE A", "NODE B", "NODE C", "EDGE A B 1", "EDGE B C 1", "TARJAN A"});

  EXPECT_EQ(output, "");
}

/*
  Тест для проверки отсортированности вывода(КСС сортируются по первой вершине)
*/
TEST(Tarjan, MultipleSccsAreSortedByFirstNode) {
  std::string output = RunCommands({"NODE A", "NODE B", "NODE D", "NODE E",
                                    "EDGE A B 1", "EDGE B A 1", "EDGE B D 1",
                                    "EDGE D E 1", "EDGE E D 1", "TARJAN A"});

  EXPECT_EQ(output, "A B\nD E\n");
}

/*
  Тест для проверки отсутствия слабо связных вершин в КСС (В примере, D не
  должно быть в КСС, содержащей C, так как они слабо связны)
*/
TEST(Tarjan, NodeOutsideCycleIsExcluded) {
  std::string output =
      RunCommands({"NODE A", "NODE B", "NODE C", "NODE D", "EDGE A B 1",
                   "EDGE B C 1", "EDGE C A 1", "EDGE C D 1", "TARJAN A"});

  EXPECT_EQ(output, "A B C\n");
}

/* Тест для проверки отсортированности вершин внутри одной КСС */
TEST(Tarjan, NodesWithinSccAreSortedAlphabetically) {
  std::string output = RunCommands({"NODE Z", "NODE X", "NODE Y", "EDGE Z X 1",
                                    "EDGE X Y 1", "EDGE Y Z 1", "TARJAN Z"});

  EXPECT_EQ(output, "X Y Z\n");
}

/* Основная функция, для запуска всех тестов */
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}