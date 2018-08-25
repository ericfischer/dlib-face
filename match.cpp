#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <errno.h>
#include <math.h>
#include <ctype.h>

struct face {
	size_t seq;
	std::string bbox;
	std::vector<std::string> landmarks;
	std::vector<float> metrics;
	std::string fname;
};

std::vector<face> subjects;
bool goodonly = false;

void usage(const char *s) {
	fprintf(stderr, "Usage: %s [-s subject ...] [candidates ...]\n", s);
}

std::string nextline(FILE *f) {
	std::string out;

	int c;
	while ((c = getc(f)) != EOF) {
		out.push_back(c);

		if (c == '\n') {
			break;
		}
	}

	return out;
}

std::string gettok(std::string &s) {
	std::string out;

	while (s.size() > 0 && s[0] != ' ') {
		out.push_back(s[0]);
		s.erase(s.begin());
	}

	if (s.size() > 0 && s[0] == ' ') {
		s.erase(s.begin());
	}

	return out;
}

face toface(std::string s) {
	std::string tok;
	face f;

	tok = gettok(s);
	f.seq = atoi(tok.c_str());

	tok = gettok(s);
	f.bbox = tok;

	for (size_t i = 0; i < 5; i++) {
		tok = gettok(s);
		f.landmarks.push_back(tok);
	}

	tok = gettok(s); // --

	for (size_t i = 0; i < 128; i++) {
		tok = gettok(s);
		f.metrics.push_back(atof(tok.c_str()));
	}

	f.fname = s;
	return f;
}

face mean(std::vector<face> inputs) {
	face out;
	size_t count = 0;

	if (inputs.size() == 0) {
		fprintf(stderr, "Trying to average empty inputs\n");
		exit(EXIT_FAILURE);
	}

	for (size_t i = 0; i < inputs.size(); i++) {
		if (i == 0) {
			out.metrics = inputs[i].metrics;
			count = 1;
		} else {
			for (size_t j = 0; j < inputs[i].metrics.size(); j++) {
				if (j >= out.metrics.size()) {
					fprintf(stderr, "%s: too many metrics\n", inputs[i].fname.c_str());
					exit(EXIT_FAILURE);
				}
				out.metrics[j] += inputs[i].metrics[j];
			}
			count++;
		}
	}

	for (size_t i = 0; i < out.metrics.size(); i++) {
		out.metrics[i] /= count;
	}

	return out;
}

void read_source(std::string s) {
	FILE *f = fopen(s.c_str(), "r");
	if (f == NULL) {
		fprintf(stderr, "%s: %s\n", s.c_str(), strerror(errno));
		exit(EXIT_FAILURE);
	}

	std::vector<face> todo;

	while (true) {
		std::string s = nextline(f);
		if (s.size() == 0) {
			break;
		}
		if (!isdigit(s[0])) {
			continue;
		}
		s.resize(s.size() - 1);

		face fc = toface(s);
		todo.push_back(fc);
	}

	face avg = mean(todo);
	avg.fname = s;

	subjects.push_back(avg);

	fclose(f);
}

size_t count = 0;
double themean = 0;
double m2 = 0;

void compare(face a, face b) {
	if (a.metrics.size() != b.metrics.size()) {
		fprintf(stderr, "%s: %s: mismatched metrics\n", a.fname.c_str(), b.fname.c_str());
		return;
	}

	double diff = 0;
	for (size_t i = 0; i < a.metrics.size(); i++) {
		diff += (a.metrics[i] - b.metrics[i]) * (a.metrics[i] - b.metrics[i]);
	}
	diff = sqrt(diff);

	// larger differences are reported but all seem to be garbage non-faces
	// I think these may have been from not filtering out # lines
	if (diff < 1.3) {
		count++;
		double delta = diff - themean;
		themean = themean + delta / count;
		double delta2 = diff - themean;
		m2 = m2 + delta * delta2;
		double stddev = sqrt(m2 / count);

#if 0
		char buf[200];
		sprintf(buf, "%01.6f", diff);
		std::string toprint = std::string(buf) + "\t" + a.fname + "\t" + a.bbox + "\t" + b.fname + "\t" + c.bbox;
#endif

		if (!goodonly || diff < themean - 3.2 * stddev) {
			printf("%01.6f\t%s\t%s\t%s\t%s\n", diff, a.fname.c_str(), a.bbox.c_str(), b.fname.c_str(), b.bbox.c_str());
			if (goodonly) {
				fflush(stdout);
			}
		}
	}
}

void read_candidates(FILE *fp) {
	while (true) {
		std::string s = nextline(fp);
		if (s.size() == 0) {
			break;
		}
		if (!isdigit(s[0])) {
			continue;
		}
		s.resize(s.size() - 1);

		face fc = toface(s);

		for (size_t i = 0; i < subjects.size(); i++) {
			compare(fc, subjects[i]);
		}
	}
}

int main(int argc, char **argv) {
	int i;
	extern int optind;
	extern char *optarg;

	std::vector<std::string> sources;

	while ((i = getopt(argc, argv, "s:g")) != -1) {
		switch (i) {
		case 's':
			sources.push_back(optarg);
			break;

		case 'g':
			goodonly = true;
			break;

		default:
			usage(argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	for (size_t i = 0; i < sources.size(); i++) {
		read_source(sources[i]);
	}

	if (subjects.size() == 0) {
		fprintf(stderr, "%s: No subjects specified\n", *argv);
		exit(EXIT_FAILURE);
	}

	if (optind == argc) {
		read_candidates(stdin);
	} else {
		for (; optind < argc; optind++) {
			FILE *f = fopen(argv[optind], "r");
			if (f == NULL) {
				fprintf(stderr, "%s: %s: %s\n", argv[0], argv[optind], strerror(errno));
				exit(EXIT_FAILURE);
			}

			read_candidates(f);
			fclose(f);
		}
	}

	fprintf(stderr, "%zu: %.6f %.6f\n", count, themean, sqrt(m2 / count));
}
