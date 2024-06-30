#include <VtkFileWriter/VtkFileWriter.h>

int main() {
    using namespace VtkFileWriter;

    std::filesystem::path outputFile = std::filesystem::current_path() / "Triangulation.vtu";

    std::vector<VtkPoint<double>> points{{{0.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {1.0, 0.0, 0.0}, {1.0, 1.0, 0.0}}};
    std::vector<VtkCell> cells{{VtkCellType::VTK_TRIANGLE, {0, 1, 2}}, {VtkCellType::VTK_TRIANGLE, {1, 2, 3}}};

    std::vector<double> pressurePointData = {0.0, 1.0, 2.0, 3.0};
    std::vector<std::array<double, 2>> velocityPointData = {{0.0, 0.0}, {0.0, 1.0}, {1.0, 0.0}, {1.0, 1.0}};

    UnstructuredVtkWriter vtkFileWriter;
    vtkFileWriter.setFile(outputFile)
        .setPoints(points)
        .setCells(cells)
        .addPointData("Pressure", std::vector<double>{{0.0, 1.0, 2.0, 3.0}})
        .addFieldData("Time", std::vector<double>{0.0})
        .addPointData("Velocity", velocityPointData);

    vtkFileWriter.write();
}
