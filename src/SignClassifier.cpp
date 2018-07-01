#include "SignClassifier.h"
#include <fstream>
#include <utility>
#include <vector>
#include <opencv2/imgproc.hpp>

SignClassifier::SignClassifier()
{
  string graph_path = tensorflow::io::JoinPath(root_dir, graph);
  Status load_graph_status = LoadGraph(graph_path, &session);
  if (!load_graph_status.ok()) {
    throw "aaaaH!!!!!";
  }

  Status read_labels_status =
      ReadLabelsFile(labelsFilePath, &labels, &label_count);
  if (!read_labels_status.ok()) {
    throw "aaaa2H!!!!!";
  }

}

Status SignClassifier::LoadGraph(const string& graph_file_name,
    std::unique_ptr<tensorflow::Session>* session) {
  tensorflow::GraphDef graph_def;
  Status load_graph_status =
      ReadBinaryProto(tensorflow::Env::Default(), graph_file_name, &graph_def);
  if (!load_graph_status.ok()) {
    return tensorflow::errors::NotFound("Failed to load compute graph at '",
        graph_file_name, "'");
  }
  session->reset(tensorflow::NewSession(tensorflow::SessionOptions()));
  Status session_create_status = (*session)->Create(graph_def);
  if (!session_create_status.ok()) {
    return session_create_status;
  }
  return Status::OK();
}

const Status SignClassifier::classify(const cv::Mat& mat, SignID& signId)
{
  cv::Mat resized;
  cv::resize(mat, resized, cv::Size(input_width, input_height));

  using namespace ::tensorflow::ops;
  //https://stackoverflow.com/questions/36044197/how-do-i-pass-an-opencv-mat-into-a-c-tensorflow-graph/36093347
  tensorflow::Tensor input_tensor(tensorflow::DT_FLOAT,
      tensorflow::TensorShape({1,resized.rows, resized.cols, 3}));

  auto input_tensor_mapped = input_tensor.tensor<float, 4>();

  for (int y = 0;y<resized.rows; ++y) {
    for(int x =0;x<resized.cols;++x) {
      cv::Vec3b pixel = resized.at<cv::Vec3b>(y, x); //BGR
      input_tensor_mapped(0, y, x, 0) = (float(pixel[2]) - 128.0)/128.0f;
      input_tensor_mapped(0, y, x, 1) = (float(pixel[1]) - 128.0)/128.0f;
      input_tensor_mapped(0, y, x, 2) = (float(pixel[0]) - 128.0)/128.0f;
    }
  }


  auto root = tensorflow::Scope::NewRootScope();
  std::vector<Tensor> out_tensors;
  std::vector<std::pair<string, tensorflow::Tensor>> inputs = {
      {"input", input_tensor},
  };
/*
  // The convention for image ops in TensorFlow is that all images are expected
  // to be in batches, so that they're four-dimensional arrays with indices of
  // [batch, height, width, channel]. Because we only have a single image, we
  // have to add a batch dimension of 1 to the start with ExpandDims().
  //auto dims_expander = tensorflow::ops::ExpandDims(root, float_caster, 0);
  // Bilinearly resize the image to fit the required dimensions.
  auto resized = tensorflow::ops::ResizeBilinear(
      root, input_tensor,
      tensorflow::ops::Const(root.WithOpName("size"), {input_height, input_width}));
  // Subtract the mean and divide by the scale.
  auto normalized = Div(root.WithOpName(output_name), Sub(root, resized, {input_mean}),
      {input_std});

  // This runs the GraphDef network definition that we've just constructed, and
  // returns the results in the output tensor.
  tensorflow::GraphDef graph;
  TF_RETURN_IF_ERROR(root.ToGraphDef(&graph));

  std::unique_ptr<tensorflow::Session> session_(
      tensorflow::NewSession(tensorflow::SessionOptions()));
  TF_RETURN_IF_ERROR(session_->Create(graph));
  TF_RETURN_IF_ERROR(session_->Run({inputs}, {output_name}, {}, &out_tensors));

*/
  const Tensor& resized_tensor = input_tensor; //out_tensors[0];






  // Actually run the image through the model.
  std::vector<Tensor> outputs;
  Status run_status = session->Run({{input_layer, resized_tensor}},
      {output_layer}, {}, &outputs);
  if (!run_status.ok()) {
    LOG(ERROR) << "Running model failed: " << run_status;
    return Status::OK();
  }


  // Given the output of a model run, and the name of a file containing the labels
  // this prints out the top five highest-scoring values.

  const int how_many_labels = std::min(5, static_cast<int>(label_count));
  Tensor indices;
  Tensor scores;
  TF_RETURN_IF_ERROR(GetTopLabels(outputs, how_many_labels, &indices, &scores));
  tensorflow::TTypes<float>::Flat scores_flat = scores.flat<float>();
  tensorflow::TTypes<int32>::Flat indices_flat = indices.flat<int32>();



  float maxScore = 0;
  int maxLabelIndex = 0;

  for (int pos = 0; pos < how_many_labels; ++pos) {
    const int label_index = indices_flat(pos);
    const float score = scores_flat(pos);
    //std::cout << labels[label_index] << " (" << label_index << "): " << score<<std::endl;

    if (score > maxScore) {
      maxScore = score;
      maxLabelIndex = label_index;
    }
  }


  //convert to sign id

  if (labels[maxLabelIndex] == "no sign") {
    signId = -1;
  } else if (labels[maxLabelIndex] == "other sign") {
    signId = -2;
  } else {
    signId = std::stoi(labels[maxLabelIndex]);
  }

  return Status::OK();
}



// Takes a file name, and loads a list of labels from it, one per line, and
// returns a vector of the strings. It pads with empty strings so the length
// of the result is a multiple of 16, because our model expects that.
Status SignClassifier::ReadLabelsFile(const string& file_name, std::vector<string>* result,
    size_t* found_label_count) {
  std::ifstream file(file_name);
  if (!file) {
    return tensorflow::errors::NotFound("Labels file ", file_name,
        " not found.");
  }
  result->clear();
  string line;
  while (std::getline(file, line)) {
    result->push_back(line);
  }
  *found_label_count = result->size();
  const int padding = 16;
  while (result->size() % padding) {
    result->emplace_back();
  }
  return Status::OK();
}

// Analyzes the output of the Inception graph to retrieve the highest scores and
// their positions in the tensor, which correspond to categories.
Status SignClassifier::GetTopLabels(const std::vector<Tensor>& outputs, int how_many_labels,
    Tensor* indices, Tensor* scores) {
  auto root = tensorflow::Scope::NewRootScope();
  using namespace ::tensorflow::ops;  // NOLINT(build/namespaces)

  string output_name = "top_k";
  TopK(root.WithOpName(output_name), outputs[0], how_many_labels);
  // This runs the GraphDef network definition that we've just constructed, and
  // returns the results in the output tensors.
  tensorflow::GraphDef graph;
  TF_RETURN_IF_ERROR(root.ToGraphDef(&graph));

  std::unique_ptr<tensorflow::Session> session(
      tensorflow::NewSession(tensorflow::SessionOptions()));
  TF_RETURN_IF_ERROR(session->Create(graph));
  // The TopK node returns two outputs, the scores and their original indices,
  // so we have to append :0 and :1 to specify them both.
  std::vector<Tensor> out_tensors;
  TF_RETURN_IF_ERROR(session->Run({}, {output_name + ":0", output_name + ":1"},
      {}, &out_tensors));
  *scores = out_tensors[0];
  *indices = out_tensors[1];
  return Status::OK();
}