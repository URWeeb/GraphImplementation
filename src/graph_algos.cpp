#include <algorithm>
#include <cstddef>
#include <functional>
#include <iostream>
#include <limits>
#include <list>
#include <map>
#include <queue>
#include <sstream>
#include <stack>
#include <string>
#include <unordered_map>

/*
  Имплементированный класс ориентированного графа

  Примечание:
    Каждый пункт будет подробно расписан непосредственно над объектом,
  соответствующим ему

  - Поля:
  -- node_storage_

  - Структуры, определённые внутри Graph:
  -- struct Node
  -- struct Edge
  -- struct ParentData
  -- struct TarjanInfo

  - Публичные методы:
  -- Graph()
  -- ~Graph()
  -- void CommandProcessing(std::stringstream&)

  - Приватные методы:
  -- std::pair<Node*, Node*> GetNodesIfPairExists(const std::string&, const
  std::string&)
  -- void DFS(Node*, std::unordered_map<Node*, int>&, std::vector<std::string>&)
  -- void AddNode(const std::string&)
  -- void AddEdge(const std::string&, const std::string&, unsigned)
  -- void RemoveEdge(Edge*)
  -- void RemoveEdge(const std::string&, const std::string&)
  -- void RemoveNode(const std::string&)
  -- void RPONumbering(const std::string&)
  -- void Dijkstra(const std::string&)
  -- void EdmondsKarp(const std::string&, const std::string&)
  -- void AugmenticBFS(Node*, Node*, std::unordered_map<Node*, ParentData>&)
  -- void SCCsSearch(const std::string&)
  -- void SCCsDFS(Node*, int&, std::unordered_map<Node*, TarjanInfo>&,
  std::stack<Node*>&, std::vector<std::vector<std::string>>&)
*/
class Graph {
private:
  struct Node;
  struct Edge;

public:
  /* Конструктор класса по умолчанию */
  Graph() = default;

  /* Деструктор класса */
  ~Graph() {
    for (auto &element : node_storage_) {
      Node *current_node = element.second;

      for (auto edge : current_node->out) {
        delete edge;
      }

      delete current_node;
    }
  }

  /* Метод для обработки комманды через буффер, построенный по соответствующей
   * строке */
  void CommandProcessing(std::stringstream &string_stream) {
    std::string command;
    string_stream >> command;

    if (command == "NODE") {
      std::string node_name;

      string_stream >> node_name;
      AddNode(node_name);
    } else if (command == "EDGE") {
      std::string first_node;
      std::string second_node;
      unsigned weight;

      string_stream >> first_node >> second_node >> weight;
      AddEdge(first_node, second_node, weight);
    } else if (command == "REMOVE") {
      std::string type;
      string_stream >> type;

      if (type == "NODE") {
        std::string node_name;
        string_stream >> node_name;

        RemoveNode(node_name);
      } else if (type == "EDGE") {
        std::string first_node;
        std::string second_node;
        string_stream >> first_node >> second_node;

        RemoveEdge(first_node, second_node);
      }
    } else if (command == "RPO_NUMBERING") {
      std::string start_node;
      string_stream >> start_node;

      RPONumbering(start_node);
    } else if (command == "DIJKSTRA") {
      std::string start_node;
      string_stream >> start_node;

      Dijkstra(start_node);
    } else if (command == "MAXFLOW") {
      std::string start_node;
      std::string end_node;
      string_stream >> start_node >> end_node;

      EdmondsKarp(start_node, end_node);
    } else if (command == "TARJAN") {
      std::string start_node;
      string_stream >> start_node;

      SCCSearch(start_node);
    }
  }

private:
  /*
    Псевдоним для итератора в std::list<Edge*>
  */
  using EdgeIterator = std::list<Edge *>::iterator;

  /*
    Структура узла графа

    - Поля:
    -- std::string id - идентификатор узла
    -- std::list<Edge*> in - двусвязный список входящих в вершину рёбер
    -- std::list<Edge*> out - двусвязный список исходящих из вершины рёбер
    -- std::unordered_map<Node*, Edge*> out_edge_table - хэш-таблица для
    быстрого поиска исходящих рёбер по имени узла;

    - Методы:
    -- Node(const std::string&) - конструктор класса по идентификатору
    -- void AddInEdge(Edge*) - метод для добавления входящего ребра в
    соответствующий список
    -- void AddOutEdge(Edge*) - метод для добавления исходящего ребра в
    соответствующий список
  */
  struct Node {
  public:
    Node(const std::string &id) : id(id) {}

    void AddInEdge(Edge *in_edge) {
      in.push_back(in_edge);
      in_edge->in_position = in.end();
      --in_edge->in_position;
    }

    void AddOutEdge(Edge *out_edge) {
      out.push_back(out_edge);
      out_edge->out_position = out.end();
      --out_edge->out_position;
      out_edge_table[out_edge->target] = out_edge;
    }

  public:
    std::string id;
    std::list<Edge *> in = {};
    std::list<Edge *> out = {};
    std::unordered_map<Node *, Edge *> out_edge_table;
  };

  /*
    Структура ребра графа

    - Поля:
    -- Node* source - вершина, из которой ребро исходит
    -- Node* target - вершина, в которую ребро входит
    -- unsigned weight - вес/пропускная способность ребра
    -- int flow - величина потока, проходящего через ребро
    -- EdgeIterator in_position - итератор, указывающий на местоположение ребра
    в списке входящих рёбер
    -- EdgeIterator in_position - итератор, указывающий на местоположение ребра
    в списке исходящих рёбер
  */
  struct Edge {
  public:
    Edge(Node *source, Node *target, unsigned weight)
        : source(source), target(target), weight(weight) {}

  public:
    Node *source;
    Node *target;
    unsigned weight;

    int flow = 0;

    EdgeIterator in_position;
    EdgeIterator out_position;
  };

  /*
    Структура для хранения данных предыдущего ребра

    - Поля:
    -- Edge* edge - указатель на предыдущее ребро
    -- bool forward_direction - прошёл ли алгоритм по направлению ребра или же в
    обратном(то есть было ли ребро обратным)
  */
  struct ParentData {
    Edge *edge;
    bool forward_direction;
  };

  /*
    Структура для хранения данных, необходимых для работы нашей имплементации
    алгоритма Тарьяна

    - Поля:
    -- int index - время первого посещения узла при обходе DFS
    -- int lowlink - минимальный index, достижимый из данного узла через дерево
    обхода и обратные рёбра в узлы, всё ещё находящиеся на стеке
    -- bool is_on_stack - находится ли узел в данный момент на стеке
  */
  struct TarjanInfo {
    int index;
    int lowlink;
    bool is_on_stack;
  };

  /*
    Метод для поиска узла графа по идентификатору. Если идентификтор не является
    валидным(то есть в графе нет такой вершины), то выводится сообщение об
    ошибке и возвращается nullptr. Если же всё корректно, то возвращается
    указатель на вершину с этим именем

    - Параметры:
    -- const std::string& node_name - идентификатор искомой ноды
  */
  Node *GetNodeIfItExists(const std::string &node_name) {
    auto node_iter = node_storage_.find(node_name);

    if (node_iter == node_storage_.end()) {
      std::cout << "Unknown node " << node_name << std::endl;
      return nullptr;
    }

    return node_iter->second;
  }

  /*
    Метод для поиска двух узлов графа по их идентификаторам с проверкой
    существования. Если хотя бы один из узлов не найден, печатает
    соответствующее сообщение об ошибке и возвращает пару указателей на nullptr.

    - Параметры:
    -- const std::string& source_name - идентификатор первого (исходного) узла
    -- const std::string& target_name - идентификатор второго (целевого) узла

    - Возвращаемое значение:
    -- std::pair<Node*, Node*> - пара указателей на найденные узлы или же пара
    nullptr, если одного из узлов не существует
  */
  std::pair<Node *, Node *>
  GetNodesIfPairExists(const std::string &source_name,
                       const std::string &target_name) {
    auto source_iter = node_storage_.find(source_name);
    auto target_iter = node_storage_.find(target_name);

    bool source_criteria = (source_iter != node_storage_.end());
    bool target_criteria = (target_iter != node_storage_.end());

    if (!source_criteria && !target_criteria) {
      std::cout << "Unknown nodes " << source_name << " " << target_name
                << std::endl;
      return {nullptr, nullptr};
    } else if (!source_criteria) {
      std::cout << "Unknown node " << source_name << std::endl;
      return {nullptr, nullptr};
    } else if (!target_criteria) {
      std::cout << "Unknown node " << target_name << std::endl;
      return {nullptr, nullptr};
    }

    return {source_iter->second, target_iter->second};
  }

  /*
    Итеративная реализация алгоритма обхода графа в глубину

    - Параметры:
    -- start_node - узел, с которого начинается обход
    -- colors - хеш-таблица для хранения цветов, соответствующих каждой вершине
    при обходе
    -- post_order_ids - вектор, в который добавляются идентификаторы узлов в
    post-order порядке
  */
  void DFS(Node *start_node, std::unordered_map<Node *, int> &colors,
           std::vector<std::string> &post_ordere_ids) {
    std::stack<std::pair<Node *, EdgeIterator>> stack;
    colors[start_node] = 1;

    stack.emplace(start_node, start_node->out.begin());

    while (!stack.empty()) {
      auto &[current_node, edge_iterator] = stack.top();

      if (edge_iterator == current_node->out.end()) {
        colors[current_node] = 2;
        post_ordere_ids.push_back(current_node->id);
        stack.pop();
        continue;
      }

      Edge *edge = *(edge_iterator++);

      switch (colors[edge->target]) {
      case 0:
        colors[edge->target] = 1;
        stack.emplace(edge->target, edge->target->out.begin());
        break;

      case 1:
        std::cout << "Found loop " << current_node->id << "->"
                  << edge->target->id << std::endl;
        break;

      default:
        break;
      }
    }
  }

  /*
    Метод для добавления новой вершины в граф

    - Параметры:
    -- const std::string& node_name - идентификатор новой ноды
  */
  void AddNode(const std::string &node_name) {
    auto [iter, success] = node_storage_.emplace(node_name, nullptr);

    if (success) {
      iter->second = new Node{node_name};
    }
  }

  /*
    Метод для добавления нового ребра в граф. Если одной из вершин в графе нет,
    выводится соответствующее сообщение об ошибке

    - Параметры:
    -- const std::string& source_name - имя исходного узла
    -- const std::string& target_name - имя целевого узла
    -- unsigned weight - вес ребра(или же его пропускная способность)
  */
  void AddEdge(const std::string &source_name, const std::string &target_name,
               unsigned weight) {
    auto [source_node, target_node] =
        GetNodesIfPairExists(source_name, target_name);

    if (!source_node || !target_node) {
      return;
    }

    auto search_iter = source_node->out_edge_table.find(target_node);
    if (search_iter != source_node->out_edge_table.end()) {
      search_iter->second->weight = weight;
      return;
    }

    Edge *new_edge = new Edge{source_node, target_node, weight};
    source_node->AddOutEdge(new_edge);
    target_node->AddInEdge(new_edge);
  }

  /*
    Метод для удаления ребра из графа по указателю.

    - Параметры:
    -- Edge* edge - указатель на удаляемое ребро
  */
  void RemoveEdge(Edge *edge) {
    if (edge == nullptr) {
      return;
    }

    edge->source->out.erase(edge->out_position);
    edge->source->out_edge_table.erase(edge->target);
    edge->target->in.erase(edge->in_position);

    delete edge;
  }

  /*
    Метод для удаления ребра из графа по идентификаторам его узлов. Если одного
    из узлов на деле не существует, выводится сообщение об ошибке


    - Параметры:
    -- const std::string& source_name - идентификатор исходного узла
    -- const std::string& target_name - идентификатор целевого узла
  */
  void RemoveEdge(const std::string &source_name,
                  const std::string &target_name) {
    auto [source_node, target_node] =
        GetNodesIfPairExists(source_name, target_name);

    if (!source_node || !target_node) {
      return;
    }

    auto search_iter = source_node->out_edge_table.find(target_node);
    if (search_iter != source_node->out_edge_table.end()) {
      RemoveEdge(search_iter->second);
    }
  }

  /*
    Метод для удаления узла из по графа по идентификатору. Если узла в графе и
    не было, выводится сообщение об ошибке

    - Параметры:
    -- const std::string& node_name - идентификатор удаляемой ноды
  */
  void RemoveNode(const std::string &node_name) {
    auto node_iter = node_storage_.find(node_name);

    if (node_iter == node_storage_.end()) {
      std::cout << "Unknown node " << node_name << std::endl;
      return;
    }

    Node *valid_node = GetNodeIfItExists(node_name);

    while (!valid_node->in.empty()) {
      RemoveEdge(valid_node->in.front());
    }

    while (!valid_node->out.empty()) {
      RemoveEdge(valid_node->out.front());
    }

    node_storage_.erase(node_iter);

    delete valid_node;
  }

  /*
    Метод для нумерации нод в порядке Reverse Post-Order(RPO), относительно
    стартовой вершины. Если стартовой вершины, на самом деле, нет в графе,
    выводится сообщение об ошибке.

    - Параметры:
    -- const std::string& start_node_name - идентификатор стартовой ноды
  */
  void RPONumbering(const std::string &start_node_name) {
    Node *start_node = GetNodeIfItExists(start_node_name);

    if (start_node == nullptr) {
      return;
    }

    std::unordered_map<Node *, int> colors = {};
    std::vector<std::string> post_order_ids = {};

    DFS(start_node, colors, post_order_ids);

    bool first_iter = true;
    for (auto reverse_iter = post_order_ids.rbegin();
         reverse_iter != post_order_ids.rend(); ++reverse_iter) {
      if (!first_iter) {
        std::cout << " ";
      } else {
        first_iter = false;
      }

      std::cout << *reverse_iter;
    }

    std::cout << std::endl;
  }

  /*
    Метод для запуска алгоритма Дейкстры относительно некоторой стартовой
    вершины. Ответ выводится в отсортированном порядке относительно имён
    узлов(поэтому и используется std::map). Если стартовой ноды с данным
    идентификатором в графе нет, выводится сообщение об ошибке

    - Параметры:
    -- const std::string& start_node_name - идентификатор стартовой ноды
  */
  void Dijkstra(const std::string &start_node_name) {
    if (GetNodeIfItExists(start_node_name) == nullptr) {
      return;
    }

    std::map<std::string, unsigned> distances;
    distances[start_node_name] = 0;

    using WeightedPair = std::pair<unsigned, Node *>;
    std::priority_queue<WeightedPair, std::vector<WeightedPair>,
                        std::greater<WeightedPair>>
        queue;

    queue.emplace(0, node_storage_[start_node_name]);

    while (!queue.empty()) {
      auto [queue_dist, queue_node] = queue.top();
      queue.pop();

      if (queue_dist <= distances[queue_node->id]) {
        for (auto edge : queue_node->out) {
          if (distances.find(edge->target->id) == distances.end() ||
              distances[edge->target->id] >
                  edge->weight + distances[queue_node->id]) {
            distances[edge->target->id] =
                edge->weight + distances[queue_node->id];
            queue.emplace(distances[edge->target->id], edge->target);
          }
        }
      }
    }

    for (auto &pair : distances) {
      if (pair.first == start_node_name) {
        continue;
      }

      std::cout << pair.first << " " << pair.second << std::endl;
    }
  }

  /*
    Метод для запуска алгоритма Эдмондса-Карпа(BFS-модификация алгоритма
    Форда-Фалкерсона поиска max-потока) относительно двух нод графа(первая -
    источник, вторая - сток). Если какая-то из нод отсутствует в графе,
    выводится соответствующее случаю сообщение об ошибке

    - Параметры:
    -- const std::string& start_point - идентификатор источника
    -- const std::string& end_point - идентификатор стока
  */
  void EdmondsKarp(const std::string &start_point,
                   const std::string &end_point) {
    auto [start_node, end_node] = GetNodesIfPairExists(start_point, end_point);

    if (!start_node || !end_node) {
      return;
    }

    if (start_node == end_node) {
      std::cout << 0 << std::endl;
      return;
    }

    for (auto &pair : node_storage_) {
      for (auto edge : pair.second->out) {
        edge->flow = 0;
      }
    }

    unsigned max_flow = 0;

    while (true) {
      std::unordered_map<Node *, ParentData> parents;

      if (!AugmenticBFS(start_node, end_node, parents)) {
        break;
      }

      Node *current_node = end_node;
      int bottleneck_value = std::numeric_limits<int>::max();

      while (current_node != start_node) {
        ParentData parent_info = parents[current_node];
        bottleneck_value =
            std::min(bottleneck_value,
                     parent_info.forward_direction
                         ? (static_cast<int>(parent_info.edge->weight) -
                            parent_info.edge->flow)
                         : parent_info.edge->flow);
        current_node = parent_info.forward_direction ? parent_info.edge->source
                                                     : parent_info.edge->target;
      }

      current_node = end_node;
      while (current_node != start_node) {
        ParentData parent_info = parents[current_node];

        if (parent_info.forward_direction) {
          parent_info.edge->flow += bottleneck_value;
          current_node = parent_info.edge->source;
        } else {
          parent_info.edge->flow -= bottleneck_value;
          current_node = parent_info.edge->target;
        }
      }

      max_flow += bottleneck_value;
    }

    std::cout << max_flow << std::endl;
  }

  /*
    Метод для запуска BFS для поиска увеличивающего пути в остаточной сети. Если
    такой путь нашёлся, метод возвращает true, иначе - false.

    - Параметры:
    -- Node* start_node - указатель на источник
    -- Node* end_node - указатель на сток
    -- std::unordered_map<Node*, ParentData>& parents - хэш-таблица для хранения
    увеличивающего пути
  */
  bool AugmenticBFS(Node *start_node, Node *end_node,
                    std::unordered_map<Node *, ParentData> &parents) {
    std::queue<Node *> queue;
    std::unordered_map<Node *, bool> visited;

    queue.push(start_node);
    visited[start_node] = true;

    while (!queue.empty()) {
      Node *current_node = queue.front();
      queue.pop();

      if (current_node == end_node) {
        return true;
      }

      for (Edge *edge : current_node->out) {
        int res_capacity = static_cast<int>(edge->weight) - edge->flow;

        if (!visited[edge->target] && res_capacity > 0) {
          visited[edge->target] = true;
          parents[edge->target] = {edge, true};

          queue.push(edge->target);
        }
      }

      for (Edge *edge : current_node->in) {
        if (!visited[edge->source] && edge->flow > 0) {
          visited[edge->source] = true;
          parents[edge->source] = {edge, false};

          queue.push(edge->source);
        }
      }
    }

    return false;
  }

  /*
    Метод для выполнения команды TARJAN относительно некоторого
    стартового узла. Если идентификатор стартовой ноды невалиден, выводит
    сообщение об ошибке

    - Параметры:
    -- const std::string& start_node_name - идентификатор стартовой ноды
  */
  void SCCSearch(const std::string &start_node_name) {
    int timer = 0;
    std::unordered_map<Node *, TarjanInfo> node_info;
    std::stack<Node *> stack;
    std::vector<std::vector<std::string>> sccs;

    Node* start_node = GetNodeIfItExists(start_node_name);
    
    if (start_node == nullptr) {
      return;
    }

    SCCDFS(start_node, timer, node_info, stack, sccs);

    std::sort(sccs.begin(), sccs.end(),
              [](const std::vector<std::string> &first,
                 const std::vector<std::string> &second) {
                return first.front() < second.front();
              });

    for (auto &scc : sccs) {
      for (size_t i = 0; i < scc.size(); ++i) {
        if (i > 0) {
          std::cout << " ";
        }

        std::cout << scc[i];
      }

      std::cout << std::endl;
    }
  }

  /*
    Метод, реализующий поиск КСС(алгоритм Тарьяна) относительно некоторой
    валидной стартовой вершины

    - Параметры:
    -- Node* start_node - указатель на валидный стартовый узел
    -- std::unordered_map<Node*, TarjanInfo>& node_info - хэш-таблица для
    хранения index/lowlink/is_on_stack для каждого посещённого узла
    -- std::stack<Node*>& stack - стек узлов-кандидатов в текущую строящуюся КСС
    -- std::vector<std::vector<std::string>>& sccs - результирующий вектор
    найденных нетривиальных КСС, которые представляются в виде векторов
    идентификаторов
  */
  void SCCDFS(Node *start_node, int &timer,
              std::unordered_map<Node *, TarjanInfo> &node_info,
              std::stack<Node *> &stack,
              std::vector<std::vector<std::string>> &sccs) {
    std::stack<std::pair<Node *, EdgeIterator>> call_stack;
    node_info[start_node] = {timer, timer, true};
    ++timer;

    stack.push(start_node);
    call_stack.emplace(start_node, start_node->out.begin());

    while (!call_stack.empty()) {
      Node *current_node = call_stack.top().first;
      EdgeIterator &edge_iterator = call_stack.top().second;

      if (edge_iterator != current_node->out.end()) {
        Edge *edge = *(edge_iterator++);

        if (node_info.find(edge->target) == node_info.end()) {
          node_info[edge->target] = {timer, timer, true};
          ++timer;

          stack.push(edge->target);
          call_stack.emplace(edge->target, edge->target->out.begin());
        } else if (node_info[edge->target].is_on_stack) {
          node_info[current_node].lowlink = std::min(
              node_info[current_node].lowlink, node_info[edge->target].index);
        }

        continue;
      }

      if (node_info[current_node].index == node_info[current_node].lowlink) {
        std::vector<std::string> current_scc;

        while (true) {
          Node *stack_node = stack.top();
          stack.pop();
          node_info[stack_node].is_on_stack = false;
          current_scc.push_back(stack_node->id);

          if (current_node == stack_node) {
            break;
          }
        }

        if (current_scc.size() > 1) {
          std::sort(current_scc.begin(), current_scc.end());
          sccs.push_back(current_scc);
        }
      }

      call_stack.pop();

      if (!call_stack.empty()) {
        node_info[call_stack.top().first].lowlink =
            std::min(node_info[call_stack.top().first].lowlink,
                     node_info[current_node].lowlink);
      }
    }
  }

private:
  std::unordered_map<std::string, Node *> node_storage_ = {};
};

/*
  Основная функция, в которой инициализируется пустой граф, а далее парсятся и
  выполняются комманды, пока есть ввод
*/
#ifndef GRAPH_ALGOS_NO_MAIN

int main() {
  std::ios_base::sync_with_stdio(false);
  std::cin.tie(NULL);

  Graph graph{};
  std::string current_line;

  while (std::getline(std::cin, current_line)) {
    if (current_line.empty()) {
      continue;
    }

    std::stringstream string_stream(current_line);
    graph.CommandProcessing(string_stream);
  }

  return 0;
}

#endif