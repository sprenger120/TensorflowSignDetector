#ifndef SIGN_DETECC_SIGNCLASSIFIER_H
#define SIGN_DETECC_SIGNCLASSIFIER_H
#include "Types.h"
#include "cv.h"
#include "tensorflow/cc/ops/const_op.h"
#include "tensorflow/cc/ops/image_ops.h"
#include "tensorflow/cc/ops/standard_ops.h"
#include "tensorflow/core/framework/graph.pb.h"
#include "tensorflow/core/framework/tensor.h"
#include "tensorflow/core/graph/default_device.h"
#include "tensorflow/core/graph/graph_def_builder.h"
#include "tensorflow/core/lib/core/errors.h"
#include "tensorflow/core/lib/core/stringpiece.h"
#include "tensorflow/core/lib/core/threadpool.h"
#include "tensorflow/core/lib/io/path.h"
#include "tensorflow/core/lib/strings/stringprintf.h"
#include "tensorflow/core/platform/env.h"
#include "tensorflow/core/platform/init_main.h"
#include "tensorflow/core/platform/logging.h"
#include "tensorflow/core/platform/types.h"
#include "tensorflow/core/public/session.h"
#include "tensorflow/core/util/command_line_flags.h"

using tensorflow::Flag;
using tensorflow::Tensor;
using tensorflow::Status;
using tensorflow::string;
using tensorflow::int32;

//Most of the code is taken from tensorflows label example

class SignClassifier {
  std::unique_ptr<tensorflow::Session> session;
  string image = "/home/sprenger/workspace/SignDetecc/train_data/3/11909.jpg";

  string graph =
      "classifier_model/retrained_graph.pb";
  string labelsFilePath =
      "classifier_model/retrained_labels.txt";

  int32 input_width = 299;
  int32 input_height = 299;
  float input_mean = 128;
  float input_std = 128;
  string input_layer = "Mul";
  string output_layer = "final_result";
  bool self_test = false;
  string root_dir = "";
  string output_name = "normalized";

  std::vector<string> labels;
  size_t label_count;

public:
  SignClassifier();
  const Status classify(const cv::Mat& mat, SignID& signId);
private:
// Reads a model graph definition from disk, and creates a session object you
// can use to run it.
  Status LoadGraph(const string& graph_file_name,
      std::unique_ptr<tensorflow::Session>* session);


// Takes a file name, and loads a list of labels from it, one per line, and
// returns a vector of the strings. It pads with empty strings so the length
// of the result is a multiple of 16, because our model expects that.
  Status ReadLabelsFile(const string& file_name, std::vector<string>* result,
      size_t* found_label_count);

  // Analyzes the output of the Inception graph to retrieve the highest scores and
// their positions in the tensor, which correspond to categories.
  Status GetTopLabels(const std::vector<Tensor>& outputs, int how_many_labels,
      Tensor* indices, Tensor* scores);

};

#endif //SIGN_DETECC_SIGNCLASSIFIER_H
