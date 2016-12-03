# jAnts #
A solver for the Capacitated Vehicle Routing Problem (CVRP).

It uses ant colony optimization with Clarke & Wright''s savings heuristic. 2-Opt and 1-Shuffle are used for intra-route optimization, whereas 1-Exchange is used for inter-route optimization.

Code written in C++11 and uses OpenMP.

## Disclaimer ##
This was a coursework completed during my CS degree at Bristol University. Any usage is allowed as long as credit is given. jAnts is inteded for people searching for ideas on solving CVRP or benchmarking their solvers. No parts of jAnts should appear in a coding coursework.

## Compiling ##
make all

## Usage ##
./jants -h

```
 ------------------------------------
 jAnts - Joy's CVRP solver using ACO
 ------------------------------------
 Usage: ./jants [COMMANDS] [OPTIONS]
        Commands:
             -h, --help
                 Print help message
             -br, --basicrand
                 Do basic random search
             -ex, --exchange
                 Do basic exchange search
             -gr, --grid
                 Do grid search on ACO with index range
        Options:
             -lg, --loglv
                 Set log level {0-4}
             -i, --input
                 Set input file
             -o, --output
                 Set output file (or "stdout")
             -dv, --divine
                 Use existing route in divine.h
             -rs, --randseed
                 Set starting RNG seed
             -tl, --timelimit
                 Set time limit in minutes
             -p, --population
                 Set population size
             -mxs, --maxstagnancy
                 Set stopping stagnant iterations
             -a, --alpha
                 Set importance of distance in ACO
             -b, --beta
                 Set importance of pheromone in ACO
             -ps, --persistence
                 Set persistence of pheromone in ACO
             -nd, --nbhooddiv
                 Set nbhood divisor in ACO
             -mnp, --minphero
                 Set min pheromone in ACO
```