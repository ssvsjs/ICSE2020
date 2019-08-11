# ICSE 2020
These files were used to calculate the model effect and select candidate comitters for the paper. In addition, `thematic-analysis-results.pdf` contains the results of the thematic analysis. 

The folder `calculate-model-effect` contains the MySQL database and the Python script for each metric.
In the MySQL database, there are the following tables:
1. scmlog (the basic information of the commits)
2. files
3. hash_file (the hash of commits and their modified files)
4. sign (signed-off-by of commits)
5. review (reviewed-by of commits)
6. test (tested-by of commits)
7. ack (acked-by of commits)
8. maintainers (created using the file MAINTAINERS in the Linux kernel repository)
9. signer_maintainer
10. i915-committer-no-maintainer

The folder `select-candidate-committers` contains the data and the C++ script for selecting candidate committers for the subsystems.
First, run `gen.cpp` to build the collaboration network of the contributors for each potential subsystem.
Then, run `main.cpp` to get the list of the candidate committers.