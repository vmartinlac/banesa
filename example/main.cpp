#include <random>
#include <opencv2/core.hpp>
#include <iostream>
#include "banesa.h"

using RandomEngine = std::default_random_engine;

class ExperimentalConditionsNode : public Node
{
public:

    ExperimentalConditionsNode(RandomEngine& random) : myRandom(random)
    {
        setName("experimental_conditions");
        registerValueFactory( std::make_shared< FileValueFactory<cv::Mat3b> >("image") );
        registerValueFactory( std::make_shared<SE3ValueFactory>("camera_to_world") );
        registerValueFactory( std::make_shared<SE3ValueFactory>("object_to_world") );
    }

    void getSample(const std::vector<ValuePtr>& input, std::vector<ValuePtr>& output) override
    {
        auto output_image = std::dynamic_pointer_cast< FileValue<cv::Mat3b> >(output[0]);
        output_image->setPath("nada.txt");
        output_image->ref() = cv::Mat3b(320, 200);

        auto output_camera_to_world = std::dynamic_pointer_cast<SE3Value>(output[1]);
        output_camera_to_world->refTranslationX() = 0.0;
        output_camera_to_world->refTranslationY() = 0.0;
        output_camera_to_world->refTranslationZ() = 0.0;
        output_camera_to_world->refQuaternionW() = 1.0;
        output_camera_to_world->refQuaternionI() = 0.0;
        output_camera_to_world->refQuaternionJ() = 0.0;
        output_camera_to_world->refQuaternionK() = 0.0;

        auto output_object_to_world = std::dynamic_pointer_cast<SE3Value>(output[2]);
        output_object_to_world->refTranslationX() = 0.0;
        output_object_to_world->refTranslationY() = 0.0;
        output_object_to_world->refTranslationZ() = 0.0;
        output_object_to_world->refQuaternionW() = 1.0;
        output_object_to_world->refQuaternionI() = 0.0;
        output_object_to_world->refQuaternionJ() = 0.0;
        output_object_to_world->refQuaternionK() = 0.0;
    }

protected:

    RandomEngine myRandom;
};

class AlgorithmParametersNode : public Node
{
public:

    AlgorithmParametersNode(RandomEngine& random) : myRandom(random)
    {
        setName("algorithm_parameters");
        registerValueFactory( std::make_shared<RealValueFactory>("threshold") );
    }

    void getSample(const std::vector<ValuePtr>& input, std::vector<ValuePtr>& output) override
    {
        auto output_threshold = std::dynamic_pointer_cast<RealValue>(output[0]);
        output_threshold->ref() = 10.0;
    }

protected:

    RandomEngine myRandom;
};

class PoseEstimationNode : public Node
{
public:

    PoseEstimationNode()
    {
        setName("pose_estimation");
        registerDependency("experimental_conditions");
        registerDependency("algorithm_parameters");
        registerValueFactory( std::make_shared<RealValueFactory>("position_est") );
    }

    void getSample(const std::vector<ValuePtr>& input, std::vector<ValuePtr>& output) override
    {
        auto input_image = std::dynamic_pointer_cast< FileValue<cv::Mat3b> >(input[0]);
        auto input_camera_to_world = std::dynamic_pointer_cast<SE3Value>(input[1]);
        auto input_object_to_world = std::dynamic_pointer_cast<SE3Value>(input[2]);
        auto input_threshold = std::dynamic_pointer_cast<RealValue>(input[3]);

        auto output_object_to_world_est = std::dynamic_pointer_cast<SE3Value>(output[0]);
    }
};

int main(int num_args, char** args)
{
    RandomEngine random;

    auto node0 = std::make_shared<ExperimentalConditionsNode>(random);
    auto node1 = std::make_shared<AlgorithmParametersNode>(random);
    auto node2 = std::make_shared<PoseEstimationNode>();

    Sampler sampler;
    sampler.run({node0, node1, node2}, 10, "db.sqlite");

    return 0;
}

