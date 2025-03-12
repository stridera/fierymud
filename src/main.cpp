#include "class.hpp"
#include "conf.hpp"
#include "db.hpp"
#include "logging.hpp"
#include "races.hpp"
#include "utils.hpp"

#include <iostream>
#include <string>
#include <unistd.h>

int main(int argc, char *argv[]) {
    int pos = 1;
    bool check_sytax = false;
    std::string dir{DFLT_DIR};
    std::string env{DFLT_ENV};

    port = DFLT_PORT;

    while ((pos < argc) && (argv[pos][0] == '-')) {
        switch (argv[pos][1]) {
        case 'd':
            if (argv[pos][2])
                dir = argv[pos] + 2;
            else if (++pos < argc)
                dir = argv[pos];
            else {
                log("Directory arg expected after option -d.");
                exit(1);
            }
            break;
        case 'e':
            if (argv[pos][2])
                env = argv[pos] + 2;
            else if (++pos < argc)
                env = argv[pos];
            else {
                log("Environment arg expected after option -e.");
                exit(1);
            }
            break;
        case 'c':
            check_sytax = true;
            log("Syntax check mode enabled.");
            break;
        case 'q':
            log("Quick boot mode.");
            break;
        case 'r':
            should_restrict = 1;
            restrict_reason = RESTRICT_ARGUMENT;
            log("Restricting game -- no new players allowed.");
            break;
        default:
            log("SYSERR: Unknown option -{:c} in argument string.", argv[pos][1]);
            break;
        }
        pos++;
    }

    if (pos < argc) {
        if (!isdigit(argv[pos][0])) {
            fprintf(stderr, "Usage: %s [-c] [-m] [-q] [-r] [-s] [-d pathname] [port #]\n", argv[0]);
            exit(1);
        } else if ((port = std::stoi(argv[pos])) <= 1024) {
            fprintf(stderr, "Illegal port number.\n");
            exit(1);
        }
    }

    if (chdir(dir.c_str()) < 0) {
        perror("Fatal error changing to data directory");
        exit(1);
    }
    log("Using {} as data directory.", dir);

    if (env == "test") {
        environment = ENV_TEST;
        log("Running in test mode.");
    } else if (env == "dev") {
        environment = ENV_DEV;
        log("Running in dev mode.");
    } else if (env == "prod") {
        environment = ENV_PROD;
        log("Running in production mode.");
    } else {
        log("Unknown environment '{}'; valid choices are 'test', 'dev', and 'prod'.", env);
        exit(1);
    }

    log("Initializing runtime game constants.");
    init_flagvectors();
    // init_rules();
    init_races();
    init_classes();
    init_objtypes();
    init_exp_table();

    if (check_sytax) {
        boot_world();
    } else {
        log("Running game on port {:d}.", port);
        init_game(port);
    }

    log("Clearing game world.");
    destroy_db();

    return 0;
}
