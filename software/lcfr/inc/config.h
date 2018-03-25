#ifndef CONFIG_H_
#define CONFIG_H_

enum config_type {
	freq,
	change_in_freq
};

typedef struct config {
	config_type type;
	unsigned int id;
	int value;
};


#endif /* CONFIG_H_ */
