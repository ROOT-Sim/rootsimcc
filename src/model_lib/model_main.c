#include <model_lib/init.h>

extern void ProcessEvent_instr(lp_id_t me, simtime_t now, unsigned event_type, const void *event_content,
    unsigned event_size, void *st);

extern bool CanEnd_instr(lp_id_t me, const void *snapshot);

static int simulation_init(int argc, char *argv[])
{
	struct simulation_configuration rs_config;
	init_args_parse(argc, argv, &rs_config);
	rs_config.dispatcher = ProcessEvent_instr;
	rs_config.committed = CanEnd_instr;
	return RootsimInit(&rs_config);
}

int main(int argc, char *argv[])
{
	if(simulation_init(argc, argv) < 0)
		return EXIT_FAILURE;

	return RootsimRun();
}
