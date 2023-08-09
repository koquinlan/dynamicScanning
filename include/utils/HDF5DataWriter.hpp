/**
 * @file HDF5DataWriter.hpp
 * @author Kyle Quinlan (kyle.quinlan@colorado.edu)
 * @brief Class for HDF5 file I/O system
 * @version 0.1
 * @date 2023-06-30
 * 
 * @copyright Copyright (c) 2023
 * 
 * @warning This class is not currently used in the project. It is a work in progress.
 * 
 */

#include "decs.hpp"

class HDF5DataWriter {
public:
    HDF5DataWriter(std::string filename) : file_(filename, H5F_ACC_TRUNC), rootGroup_(file_.createGroup("/")) {}

    void saveData(fftw_complex* rawData, 
                //   fftw_complex* fftData, 
                  std::string groupName,
                  hsize_t numSamples) {

        H5::Group* group = new H5::Group( file_.createGroup( groupName.c_str() ));

        hsize_t dims[2] = {2, numSamples};
        H5::DataSpace dataspace(2, dims);

        std::string datasetName = groupName + "/rawData";
        H5::DataSet rawDataSet = file_.createDataSet(datasetName, H5::PredType::NATIVE_DOUBLE, dataspace);
        // H5::DataSet fftDataSet = experimentGroup.createDataSet("FFTData", H5::PredType::NATIVE_DOUBLE, dataspace);

        rawDataSet.write(rawData, H5::PredType::NATIVE_DOUBLE);
        // fftDataSet.write(fftData.data(), H5::PredType::NATIVE_DOUBLE);
    }

private:
    H5::H5File file_;
    H5::Group rootGroup_;
};



void saveDataToHDF5(SharedDataBasic& sharedData, SynchronizationFlags& syncFlags) {
    HDF5DataWriter writer("test.h5");
    int numSaved = 0;

    while (true) {
        // Check if data queue is empty
        std::unique_lock<std::mutex> lock(sharedData.mutex);
        sharedData.saveReadyCondition.wait(lock, [&sharedData]() {
            return !sharedData.dataSavingQueue.empty();
        });


        // Process data if the data queue is not empty
        while (!sharedData.dataSavingQueue.empty()) {
            // Get the pointer to the data from the queue
            fftw_complex* rawData = sharedData.dataSavingQueue.front();
            sharedData.dataSavingQueue.pop();
            lock.unlock();
        
            // Save the data to HDF5
            writer.saveData(rawData, "testGroup", sharedData.samplesPerBuffer);
            numSaved++;

            // Free the memory allocated for the raw data
            fftw_free(rawData);

            lock.lock();  // Lock again before checking the data queue
        }

        // Check if the acquisition and saving is complete
        {
            std::lock_guard<std::mutex> lock(syncFlags.mutex);
            if (syncFlags.acquisitionComplete && sharedData.dataSavingQueue.empty()) {
                break;
            }
        }
    }
}