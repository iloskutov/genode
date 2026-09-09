/*
 * \brief  Test procfs emulation of libc
 * \author Johannes Schlatow
 * \date   2026-09-09
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>

int child_main(int argc, char **argv)
{
	char *path = argv[1];
	char target[16];

	ssize_t bytes = readlink(path, target, sizeof(target));
	if (bytes >= 0 && (size_t)bytes < sizeof(target))
		target[bytes] = '\0';
	else {
		printf("Error: readlink(%s) failed\n", path);
		return -1;
	}

	printf("%s -> %s\n", path, target);
	if (strncmp(target, "/pipe/", 6) != 0) {
		printf("Error: Unexpected symlink target for %s\n", path);
		return -1;
	}

	int fd = open(path, O_RDONLY);
	if (fd < 0) {
		printf("Error: Unable to open %s\n", path);
		return -1;
	}

	char buf[16];

	bytes = read(fd, buf, sizeof(buf));
	if (bytes <= 0) {
		printf("Error: reading failed\n");
		close(fd);
		return -1;
	}

	if ((size_t)bytes >= sizeof(buf))
		bytes = sizeof(buf) - 1;

	buf[bytes] = '\n';
	printf("%s\n", buf);

	close(fd);
	return 0;
}

int main(int argc, char **argv)
{
	char path[16];
	char target[16];

	/* readlink /proc/self/fd/{0-2} */
	for (unsigned i=0; i < 3; i++) {
		snprintf(path, sizeof(path), "/proc/self/fd/%u", i);

		ssize_t bytes = readlink(path, target, sizeof(target));
		if (bytes >= 0 && (size_t)bytes < sizeof(target))
			target[bytes] = '\0';
		else {
			printf("Error: readlink(%s) failed\n", path);
			return -1;
		}

		printf("%s -> %s\n", path, target);
	}

	if (argc > 1)
		return child_main(argc, argv);

	/* create pipe */
	int pipefds[2];
	if (pipe(pipefds) < 0) {
		printf("Error: pipe() failed\n");
		return -1;
	}

	int const fdnum = 63;

	/* move to pipe's read-end to fdnum */
	if (dup2(pipefds[0], fdnum) != fdnum) {
		printf("Error: dup2() failed\n");
		return -1;
	}
	close(pipefds[0]);

	/* fork */
	pid_t fork_ret = fork();
	if (fork_ret < 0) {
		printf("Error: fork returned %d, errno=%d\n", fork_ret, errno);
		return -1;
	}

	if (fork_ret == 0) {
		char argv0[20];
		char argv1[20];

		snprintf(argv0, sizeof(argv0), "test-libc_procfs");
		snprintf(argv1, sizeof(argv1), "/dev/fd/%d", fdnum);

		char *argv[] { argv0, argv1, NULL };

		execve("test-libc_procfs", argv, NULL);
		printf("Error: execve failed\n");
	}

	/* write to pipe */
	static char const *message = { "Hello!" };
	write(pipefds[1], message, sizeof(message));

	int child_status = 0;
	waitpid(fork_ret, &child_status, 0);
	if (child_status != 0)
		return -1;

	return 0;
}
