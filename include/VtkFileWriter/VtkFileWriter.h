#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <utility>
#include <string>
#include <unordered_map>
#include <vector>
#include <type_traits>


#include <tinyxml2.h>

namespace VtkFileWriter {
    enum class VtkCellType : int {
        VTK_VERTEX = 1,
        VTK_POLY_VERTEX = 2,
        VTK_LINE = 3,
        VTK_POLY_LINE = 4,
        VTK_TRIANGLE = 5,
        VTK_TRIANGLE_STRIP = 6,
        VTK_POLYGON = 7,
        VTK_PIXEL = 8,
        VTK_QUAD = 9,
        VTK_TETRA = 10,
        VTK_VOXEL = 11,
        VTK_HEXAHEDRON = 12,
        VTK_WEDGE = 13,
        VTK_PYRAMID = 14,

        VTK_QUADRATIC_EDGE = 21,
        VTK_QUADRATIC_TRIANGLE = 22,
        VTK_QUADRATIC_QUAD = 23,
        VTK_QUADRATIC_TETRA = 24,
        VTK_QUADRATIC_HEXAHEDRON = 25
    };

    enum class VtkDataType {
        Int8,
        UInt8,
        Int16,
        UInt16,
        Int32,
        UInt32,
        Int64,
        UInt64,
        Float32,
        Float64
    };

    template <typename T>
    struct VtkPoint {
        T x, y, z;
    };

    struct VtkCell {
        VtkCellType cellType;
        std::vector<std::size_t> points;
    };

    namespace internal {
        const std::unordered_map<VtkDataType, std::string> dataTypeNameMap = {
            {VtkDataType::Int8, "Int8"},     {VtkDataType::UInt8, "UInt8"}, {VtkDataType::Int16, "Int16"},   {VtkDataType::UInt16, "UInt16"},   {VtkDataType::Int32, "Int32"},
            {VtkDataType::UInt32, "UInt32"}, {VtkDataType::Int64, "Int64"}, {VtkDataType::UInt64, "UInt64"}, {VtkDataType::Float32, "Float32"}, {VtkDataType::Float64, "Float64"},
        };

        struct DataWrapper {
            std::string name;
            VtkDataType dataType;
            std::size_t totalDataPoints;
            std::size_t numberOfComponents;

            virtual std::string contentToString() const = 0;

            virtual ~DataWrapper() = default;
        };

        template <typename T>
        struct DataWrapperImpl : public DataWrapper {
            std::vector<T> values;

            std::string contentToString() const override {
                std::string content;
                for (const T& v : values) {
                    content += std::to_string(v) + ' ';
                }
                content.pop_back();
                return content;
            }
        };

        template <typename T>
        using plainType = std::remove_cv_t<std::remove_reference_t<T>>;

        template <typename S, typename T>
        inline constexpr bool isSame = std::is_same<plainType<S>, plainType<T>>::value;

        template <typename T>
        constexpr inline VtkDataType getVtkDataType() {
            if constexpr (isSame<T, int8_t>) {
                return VtkDataType::Int8;
            } else if constexpr (isSame<T, uint8_t>) {
                return VtkDataType::UInt8;
            } else if constexpr (isSame<T, int16_t>) {
                return VtkDataType::Int16;
            } else if constexpr (isSame<T, uint16_t>) {
                return VtkDataType::UInt16;
            } else if constexpr (isSame<T, int32_t>) {
                return VtkDataType::Int32;
            } else if constexpr (isSame<T, uint32_t>) {
                return VtkDataType::Int32;
            } else if constexpr (isSame<T, int64_t>) {
                return VtkDataType::Int64;
            } else if constexpr (isSame<T, uint64_t>) {
                return VtkDataType::UInt64;
            } else if constexpr (isSame<T, float>) {
                return VtkDataType::Float32;
            } else if constexpr (isSame<T, double>) {
                return VtkDataType::Float64;
            } else {
                return VtkDataType::Float64;
            }
        }

        class UnstructuredVtkXmlWriter {
          public:
            void writeToXml(const std::filesystem::path& path, const DataWrapper& points, const std::vector<VtkCell>& cells, const std::vector<DataWrapper*>& pointData,
                            const std::vector<DataWrapper*>& cellData, const std::vector<DataWrapper*>& fieldData) {
                tinyxml2::XMLDocument doc;
                doc.InsertFirstChild(doc.NewDeclaration(NULL));

                auto vtkFileNode = doc.NewElement("VTKFile");
                vtkFileNode->SetAttribute("type", "UnstructuredGrid");
                vtkFileNode->SetAttribute("version", "0.1");
                doc.InsertEndChild(vtkFileNode);

                auto unstructuredGridNode = doc.NewElement("UnstructuredGrid");
                vtkFileNode->InsertEndChild(unstructuredGridNode);

                auto fieldNode = doc.NewElement("FieldData");
                unstructuredGridNode->InsertFirstChild(fieldNode);
                writeDataWrappers(doc, fieldNode, fieldData);

                auto pieceNode = doc.NewElement("Piece");
                unstructuredGridNode->InsertEndChild(pieceNode);

                auto pointsData = doc.NewElement("Points");
                pieceNode->InsertEndChild(pointsData);
                pieceNode->SetAttribute("NumberOfPoints", std::to_string(points.totalDataPoints / points.numberOfComponents).c_str());
                pieceNode->SetAttribute("NumberOfCells", std::to_string(cells.size()).c_str());

                auto pointsDataArray = doc.NewElement("DataArray");
                pointsData->InsertEndChild(pointsDataArray);
                pointsDataArray->SetAttribute("type", internal::dataTypeNameMap.at(points.dataType).c_str());
                pointsDataArray->SetAttribute("NumberOfComponents", points.numberOfComponents);
                pointsDataArray->SetAttribute("format", "ascii");
                pointsDataArray->SetText(points.contentToString().c_str());

                auto cellElement = doc.NewElement("Cells");
                pieceNode->InsertEndChild(cellElement);
                writeCells(doc, cellElement, cells);

                auto pointDataElement = doc.NewElement("PointData");
                pieceNode->InsertEndChild(pointDataElement);
                writeDataWrappers(doc, pointDataElement, pointData);

                auto cellDataElement = doc.NewElement("CellData");
                pieceNode->InsertEndChild(cellDataElement);
                writeDataWrappers(doc, cellDataElement, cellData);

                doc.SaveFile(path.string().c_str());
            }

          private:
            void writeCells(tinyxml2::XMLDocument& doc, tinyxml2::XMLElement* cellElement, const std::vector<VtkCell>& cells) {
                auto connectivity = doc.NewElement("DataArray");
                cellElement->InsertEndChild(connectivity);
                connectivity->SetAttribute("type", "Int32");
                connectivity->SetAttribute("Name", "connectivity");
                connectivity->SetAttribute("format", "ascii");

                auto offsets = doc.NewElement("DataArray");
                cellElement->InsertEndChild(offsets);
                offsets->SetAttribute("type", "Int32");
                offsets->SetAttribute("Name", "offsets");
                offsets->SetAttribute("format", "ascii");

                auto types = doc.NewElement("DataArray");
                cellElement->InsertEndChild(types);
                types->SetAttribute("type", "Int32");
                types->SetAttribute("Name", "types");
                types->SetAttribute("format", "ascii");

                std::string connectivityString, offsetString, typesString;
                std::size_t cumulativeOffset = 0;
                for (const VtkCell& cell : cells) {
                    for (const auto& p : cell.points) {
                        connectivityString += std::to_string(p) + ' ';
                    }

                    cumulativeOffset += cell.points.size();
                    offsetString += std::to_string(cumulativeOffset) + ' ';

                    typesString += std::to_string((int)cell.cellType) + ' ';
                }
                connectivityString.pop_back();
                offsetString.pop_back();
                typesString.pop_back();

                connectivity->SetText(connectivityString.c_str());
                offsets->SetText(offsetString.c_str());
                types->SetText(typesString.c_str());
            }

            void writeDataWrappers(tinyxml2::XMLDocument& doc, tinyxml2::XMLElement* xmlElement, const std::vector<DataWrapper*>& dataWrappers) {
                for (const DataWrapper* const dataWrapper : dataWrappers) {
                    auto dataElement = doc.NewElement("DataArray");
                    dataElement->SetAttribute("type", dataTypeNameMap.at(dataWrapper->dataType).c_str());
                    dataElement->SetAttribute("Name", dataWrapper->name.c_str());
                    dataElement->SetAttribute("NumberOfTuples", dataWrapper->totalDataPoints);
                    dataElement->SetAttribute("NumberOfComponents", dataWrapper->numberOfComponents);
                    dataElement->SetAttribute("format", "ascii");
                    dataElement->SetText(dataWrapper->contentToString().c_str());

                    xmlElement->InsertEndChild(dataElement);
                }
            }
        };

    }  // namespace internal

    class UnstructuredVtkWriter {
      public:
        /**
         * @brief Set the output file path where the VTK file will be written to. Will overwrite any path already set.
         * 
         * @param path 
         * @return UnstructuredVtkWriter& 
         */
        UnstructuredVtkWriter& setFile(std::filesystem::path& path);

        /**
         * @brief Set the points of the VTK file. Will overwrite any points already set.
         * 
         * @tparam T 
         * @param points 
         * @return UnstructuredVtkWriter& 
         */
        template <typename T>
        UnstructuredVtkWriter& setPoints(const std::vector<VtkPoint<T>>& points);

        /**
         * @brief Set the cells of the VTK file. Will overwrite any cells already set.
         * 
         * @param cells 
         * @return UnstructuredVtkWriter& 
         */
        UnstructuredVtkWriter& setCells(std::vector<VtkCell>&& cells);

        /**
         * @brief Set the cells of the VTK file. Will overwrite any cells already set.
         * 
         * @param cells 
         * @return UnstructuredVtkWriter& 
         */
        UnstructuredVtkWriter& setCells(const std::vector<VtkCell>& cells);

        /**
         * @brief Add a point data set which will be written to the VTK file.
         * 
         * @tparam T 
         * @param name Name of the point data set
         * @param pointData 
         * @return UnstructuredVtkWriter& 
         */
        template <typename T>
        UnstructuredVtkWriter& addPointData(const std::string& name, const std::vector<T>& pointData);

        /**
         * @brief Add a point data set which will be written to the VTK file.
         * 
         * @tparam T 
         * @param name Name of the point data set
         * @param pointData 
         * @return UnstructuredVtkWriter& 
         */
        template <typename T, std::size_t N>
        UnstructuredVtkWriter& addPointData(const std::string& name, const std::vector<std::array<T, N>>& pointData);

        /**
         * @brief Add a cell data set which will be written to the VTK file.
         * 
         * @tparam T 
         * @param name Name of the cell data set
         * @param addCellData 
         * @return UnstructuredVtkWriter& 
         */
        template <typename T>
        UnstructuredVtkWriter& addCellData(const std::string& name, const std::vector<T>& cellData);

        /**
         * @brief Add a cell data set which will be written to the VTK file.
         * 
         * @tparam T 
         * @param name Name of the cell data set
         * @param addCellData 
         * @return UnstructuredVtkWriter& 
         */
        template <typename T, std::size_t N>
        UnstructuredVtkWriter& addCellData(const std::string& name, const std::vector<std::array<T, N>>& cellData);

        /**
         * @brief Add a field data set which will be written to the VTK file.
         * 
         * @tparam T 
         * @param name Name of the field data set
         * @param fieldData 
         * @return UnstructuredVtkWriter& 
         */
        template <typename T>
        UnstructuredVtkWriter& addFieldData(const std::string& name, const std::vector<T>& fieldData);

        /**
         * @brief 
         * 
         */
        void write();

        void clear();

      private:
        template <typename T>
        std::unique_ptr<internal::DataWrapper> createDataWrapper(const std::string& name, int numberOfComponents, const std::vector<T>& data);

        std::unique_ptr<std::filesystem::path> outputPath_;
        std::unique_ptr<internal::DataWrapper> points_;
        std::unique_ptr<std::vector<VtkCell>> cells_;

        std::vector<std::unique_ptr<internal::DataWrapper>> pointData_;
        std::vector<std::unique_ptr<internal::DataWrapper>> cellData_;
        std::vector<std::unique_ptr<internal::DataWrapper>> fieldData_;
    };

    UnstructuredVtkWriter& UnstructuredVtkWriter::setFile(std::filesystem::path& path) {
        outputPath_ = std::make_unique<std::filesystem::path>(std::forward<std::filesystem::path>(path));

        return *this;
    }

    template <typename T>
    UnstructuredVtkWriter& UnstructuredVtkWriter::setPoints(const std::vector<VtkPoint<T>>& points) {
        std::vector<T> values;
        values.reserve(points.size());
        for (const VtkPoint<T>& point : points) {
            values.push_back(point.x);
            values.push_back(point.y);
            values.push_back(point.z);
        }

        points_ = createDataWrapper("Points", 3, values);

        return *this;
    }

    UnstructuredVtkWriter& UnstructuredVtkWriter::setCells(std::vector<VtkCell>&& cells) {
        cells_ = std::make_unique<std::vector<VtkCell>>(std::move(cells));
        return *this;
    }

    UnstructuredVtkWriter& UnstructuredVtkWriter::setCells(const std::vector<VtkCell>& cells) {
        cells_ = std::make_unique<std::vector<VtkCell>>(cells);
        return *this;
    }

    template <typename T>
    UnstructuredVtkWriter& UnstructuredVtkWriter::addPointData(const std::string& name, const std::vector<T>& pointData) {
        auto dataWrapper = createDataWrapper(name, 1, pointData);
        pointData_.push_back(std::move(dataWrapper));
        return *this;
    }

    template <typename T, std::size_t N>
    UnstructuredVtkWriter& UnstructuredVtkWriter::addPointData(const std::string& name, const std::vector<std::array<T, N>>& pointData) {
        std::vector<T> values;
        for (const std::array<T, N>& arr : pointData) {
            for (const T& v : arr) {
                values.push_back(v);
            }
        }
       auto dataWrapper = createDataWrapper(name, N, values);
       pointData_.push_back(std::move(dataWrapper));

        return *this;
    }

    template <typename T>
    UnstructuredVtkWriter& UnstructuredVtkWriter::addCellData(const std::string& name, const std::vector<T>& cellData) {
        cellData_.push_back(createDataWrapper(name, 1, cellData));
        return *this;
    }

    template <typename T, std::size_t N>
    UnstructuredVtkWriter& UnstructuredVtkWriter::addCellData(const std::string& name, const std::vector<std::array<T, N>>& cellData) {
        std::vector<T> values;
        for (const std::array<T, N>& arr : cellData) {
            for (const T& v : arr) {
                values.push_back(v);
            }
        }
        cellData_.push_back(createDataWrapper(name, N, values));

        return *this;
    }

    template <typename T>
    UnstructuredVtkWriter& UnstructuredVtkWriter::addFieldData(const std::string& name, const std::vector<T>& fieldData) {
        auto dataWrapper = createDataWrapper(name, 1, fieldData);
        fieldData_.push_back(std::move(dataWrapper));
        return *this;
    }

    void UnstructuredVtkWriter::write() {
        internal ::UnstructuredVtkXmlWriter xmlWriter{};
        std::vector<internal::DataWrapper*> pointData;
        std::vector<internal::DataWrapper*> cellData;
        std::vector<internal::DataWrapper*> fieldData;

        for (const auto& p : pointData_) {
            pointData.push_back(p.get());
        }
        for (const auto& c : cellData_) {
            cellData.push_back(c.get());
        }
        for (const auto& f : fieldData_) {
            fieldData.push_back(f.get());
        }

        xmlWriter.writeToXml(*outputPath_, *points_, *cells_, pointData, cellData, fieldData);
    }

    inline void UnstructuredVtkWriter::clear() {
        outputPath_.reset();
        points_.reset();
        cells_.reset();
        pointData_.clear();
        cellData_.clear();
    }

    template <typename T>
    std::unique_ptr<internal::DataWrapper> UnstructuredVtkWriter::createDataWrapper(const std::string& name, int numberOfComponents, const std::vector<T>& data) {
        std::unique_ptr<internal::DataWrapper> dataWrapper = std::unique_ptr<internal::DataWrapper>(new internal::DataWrapperImpl<T>());
        internal::DataWrapperImpl<T>& dataWrapperImpl = *static_cast<internal::DataWrapperImpl<T>*>(dataWrapper.get());

        dataWrapperImpl.name = name;
        dataWrapperImpl.numberOfComponents = numberOfComponents;
        dataWrapperImpl.totalDataPoints = data.size();
        dataWrapperImpl.dataType = internal::getVtkDataType<T>();
        dataWrapperImpl.values = data;

        return std::move(dataWrapper);
    }

}  // namespace VtkFileWriter
