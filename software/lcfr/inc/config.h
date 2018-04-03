#ifndef CONFIG_H_
#define CONFIG_H_

enum config_type {
	lower_freq,
	change_in_freq
};

typedef struct config {
	enum config_type type;
	unsigned int id;
	int value;
} config;


#endif /* CONFIG_H_ */
