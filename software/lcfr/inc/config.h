#ifndef CONFIG_H_
#define CONFIG_H_

enum config_type {
	upper_freq,
	lower_freq,
	change_in_freq
};

typedef struct config {
	config_type type;
	unsigned int id;
	int value;
};


#endif /* CONFIG_H_ */
