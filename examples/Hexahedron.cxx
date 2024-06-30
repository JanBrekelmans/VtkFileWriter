#include <VtkFileWriter/VtkFileWriter.h>

int main() {
    using namespace VtkFileWriter;

    std::filesystem::path outputFile = std::filesystem::current_path() / "Hexahedron.vtu";

    std::vector<VtkPoint<double>> points{{{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {1.0, 1.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, {1.0, 0.0, 1.0}, {1.0, 1.0, 1.0}, {0.0, 1.0, 1.0}}};
    std::vector<VtkCell> cells{{VtkCellType::VTK_HEXAHEDRON, {0, 1, 2,3,4,5,6,7}}};

    std::vector<double> pointData = {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0};

    UnstructuredVtkWriter vtkFileWriter;
    vtkFileWriter.setFile(outputFile)
        .setPoints(points)
        .setCells(cells)
        .addPointData("PointData", pointData);

    vtkFileWriter.write();
}
