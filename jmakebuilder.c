#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#define MAX_FILES 1000
#define MAX_FILE_LEN 100
#define MAX_PATH_LEN 1000
#define MAX_ALIAS_LEN 100

typedef struct
{
	char name [MAX_FILE_LEN];
	char path [MAX_PATH_LEN];
	char type;
} file_t;

typedef struct
{
    char alias[MAX_ALIAS_LEN];
    char path[MAX_PATH_LEN];
} route_t;

char * get_path_alias (route_t *routes, int *n_routes, char *path)
{
	// Obtener el nombre de la última carpeta en la ruta
	char *last_folder = strrchr(path, '/');
	if (last_folder == NULL)
	{
		last_folder = path;
	}
	else
	{
		last_folder++;
	}

    // Crear el alias
    char alias[MAX_ALIAS_LEN];
    sprintf(alias, "DIR-%04d-VAR", *n_routes);

    // Verificar si la ruta ya existe en el array de rutas
    for (int i = 0; i < *n_routes; i++)
	{
        if (strcmp(routes[i].path, path) == 0)
		{
            return routes[i].alias; // La ruta ya existe, devolver el alias correspondiente
        }
    }

    // Si la ruta no existe, añadirla al array de rutas
    if (*n_routes < MAX_PATH_LEN)
	{
        strcpy(routes[*n_routes].alias, alias);
        strcpy(routes[*n_routes].path, path);
        (*n_routes)++;
        return routes[*n_routes].alias;
    }
	else
	{
        printf("Se ha alcanzado el límite máximo de rutas.\n");
        exit(EXIT_FAILURE);
    }
}

void list_files(char * path, file_t * files, int * n_files)
{
	DIR * dir;
	struct dirent * ent;
	char filepath [MAX_PATH_LEN];
	dir = opendir (path);

    if (dir == NULL)
	{
		perror ("opendir");
		exit (EXIT_FAILURE);
    }

	while ((ent = readdir(dir)) != NULL)
	{
		if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
		{
			continue;
		}

		snprintf(filepath, MAX_PATH_LEN, "%s/%s", path, ent->d_name);
		if (ent->d_type == DT_DIR)
		{
			list_files(filepath, files, n_files); // Si es un directorio, llamar recursivamente a la función
		}
		else if (ent->d_type == DT_REG)
		{
			char* ext = strrchr(ent->d_name, '.'); // Obtener la extensión del archivo
			if (ext != NULL)
			{
				if ((strcmp(ext, ".c") == 0) || (strcmp(ext, ".h") == 0) || (strcmp(ext, ".cpp") == 0))
				{
					if (*n_files < MAX_FILES)
					{
						char * file = strrchr(filepath, '/');
						long length = (long)file - (long)filepath;
						strncpy(files[*n_files].path, filepath, length);

						strcpy(files[*n_files].name, ent->d_name);

						if (strcmp(ext, ".c") == 0)
							files[*n_files].type = 'c';
						else if (strcmp(ext, ".h") == 0)
							files[*n_files].type = 'h';
						else if (strcmp(ext, ".cpp") == 0)
							files[*n_files].type = 'p';
						else
							files[*n_files].type = 'x';

						(*n_files)++;
					}
					else
					{
						printf("Se ha alcanzado el límite máximo de archivos.\n");
						closedir(dir);
						exit(EXIT_FAILURE);
					}
				}
			}
		}
	}
	closedir(dir);
}

int create_make (char *path, route_t *routes, int *n_routes, file_t * files, int * n_files)
{
	FILE * make_file;
	char make_path [MAX_PATH_LEN + 25];
	char * alias;

    sprintf(make_path, "%s/%s", path, "Makefile.temp");

    make_file = fopen (make_path, "w");



	fprintf (make_file, "####################");
	fprintf (make_file, "\n# DEFINITIONS");
	fprintf (make_file, "\n####################");
	fprintf (make_file, "\n");
	fprintf (make_file, "\n# MAIN CONFIGURATION");
	fprintf (make_file, "\nCOMPILER-VAR = g++");
	fprintf (make_file, "\nCFLAGS-VAR =");
	for (int i = 0; i < *n_routes; i++)
	{
		fprintf (make_file, " -I$(%s)", routes[i].alias);
	}
	fprintf (make_file, "\nARMGCC_DIR= ~/toolchain/var-mcuxpresso/gcc-arm-none-eabi-10.3-2021.10");
	fprintf (make_file, "\n# gcc -B $ARMGCC_DIR archivo.c -o archivo_compilado");
	fprintf (make_file, "\n");
	fprintf (make_file, "\n# EXTERNAL LIBRARIES IF NEEDED");
	fprintf (make_file, "\nLIBS-VAR = # -lm");
	fprintf (make_file, "\nDIR-LIBS-VAR = ./lib");
	fprintf (make_file, "\n");
	fprintf (make_file, "\nTARGET-VAR = exe");

	fprintf (make_file, "\n");

	fprintf (make_file, "\n# DEBUG CONFIGURATION");
	fprintf (make_file, "\n# 0: NO DEBUG");
	fprintf (make_file, "\n# 1: MINIMAL");
	fprintf (make_file, "\n# 2: DEFAULT (IF EMPTY)");
	fprintf (make_file, "\n# 3: MAXIMAL");
	fprintf (make_file, "\nDEBUG = 3");
	fprintf (make_file, "\n");
	fprintf (make_file, "\n# COMPILATION PATHS");
	fprintf (make_file, "\nDIR-TARGET-VAR = ./deploy");
	fprintf (make_file, "\nDIR-OBJ-VAR = %s/.obj", path);

	for (int i = 0; i < *n_routes; i++)
		fprintf (make_file, "\n%s = %s", routes[i].alias, routes[i].path);

	fprintf (make_file, "\n");
	fprintf (make_file, "\n####################");
	fprintf (make_file, "\n# ELEMENTS");
	fprintf (make_file, "\n####################");

	fprintf (make_file, "\n");
	fprintf (make_file, "\n# LIST OF HEADER FILES");

	fprintf (make_file, "\nDEPS-VAR =");
	for (int i = 0; i < *n_files; i++)
		if (files[i].type == 'h')
		{
			alias = get_path_alias (routes, n_routes, files[i].path);
			fprintf (make_file, " \\\n\t$(%s)/%s", alias, files[i].name);
		}

	fprintf (make_file, "\n");
	fprintf (make_file, "\n# LIST OF SOURCE FILES");
	fprintf (make_file, "\nSRC-VAR =");
	for (int i = 0; i < *n_files; i++)
		if ((files[i].type == 'c') || (files[i].type == 'p'))
			fprintf (make_file, " \\\n\t%s", files[i].name);

	fprintf (make_file, "\n");
	fprintf (make_file, "\n# LIST OF OBJECT FILES");
	fprintf (make_file, "\nOBJ-VAR = $(patsubst %%.c,$(DIR-OBJ-VAR)/%%.o,$(SRC-VAR)) $(patsubst %%.cpp,$(DIR-OBJ-VAR)/%%.o,$(SRC-VAR))");

	fprintf (make_file, "\n");
	fprintf (make_file, "\n####################");
	fprintf (make_file, "\n# MAKE RULES");
	fprintf (make_file, "\n####################");
	fprintf (make_file, "\n");
	fprintf (make_file, "\n# DEFAULT RULE FOR MAKE");
	fprintf (make_file, "\n.PHONY: all");
	fprintf (make_file, "\nall: $(DIR-TARGET-VAR)/$(TARGET-VAR)");
	fprintf (make_file, "\n");
	fprintf (make_file, "\n# MAIN COMPILATION");
	fprintf (make_file, "\n$(DIR-TARGET-VAR)/$(TARGET-VAR): $(OBJ-VAR)");
	fprintf (make_file, "\n\techo \"Compilation\"");
	fprintf (make_file, "\n\tmkdir -p $(DIR-TARGET-VAR)");
	fprintf (make_file, "\n\t$(COMPILER-VAR) -g$(DEBUG) -o $@ $^ $(DEPS-VAR) $(CFLAGS-VAR) $(LIBS-VAR)");
	fprintf (make_file, "\n");
	fprintf (make_file, "\n# OBJECT COMPILATION");
		for (int i = 0; i < *n_files; i++)
		if ((files[i].type == 'c') || (files[i].type == 'p'))
		{
			fprintf (make_file, "\n");
			alias = get_path_alias (routes, n_routes, files[i].path);

			char * file = strrchr(files[i].name, '.');
			long length = (long)file - (long)&files[i].name;
			char name_aux [MAX_FILE_LEN] = "\0";
			strncpy(name_aux, files[i].name, length);

			fprintf (make_file, "\n$(DIR-OBJ-VAR)/%s.o: $(%s)/%s $(DEPS-VAR)", name_aux, alias, files[i].name);
			fprintf (make_file, "\n\tmkdir -p $(DIR-OBJ-VAR)");
			fprintf (make_file, "\n\t$(COMPILER-VAR) -g$(DEBUG) -c -o $@ $< $(CFLAGS-VAR)");



		}




	fclose (make_file);

	return 0;
}

int main (int argc, char** argv)
{
	char * path;

	if (argc < 2)
	{
		printf("Uso: %s <ruta de la carpeta>\n", argv[0]);
		return EXIT_FAILURE;
    }
    path = argv[1];

	file_t files [MAX_FILES];
    int n_files = 0;
    list_files(path, files, &n_files);

	route_t routes[MAX_PATH_LEN];
    int n_routes = 0;

	for (int i = 0; i < n_files; i++)
	{
		get_path_alias (routes, &n_routes, files[i].path);
	}

	printf ("\nLibrerias detectadas:");
    for (int i = 0; i < n_files; i++)
	{
		if (files[i].type == 'h')
		{
			printf ("\n-- .h --> %s/%s", files[i].path ,files[i].name);

			char *alias = get_path_alias (routes, &n_routes, files[i].path);
		}
	}

	printf ("\n++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");

	printf ("\nCodigo .c detectado:");
	for (int i = 0; i < n_files; i++)
	{
		if (files[i].type == 'c') 
			printf ("\n-- .c --> %s/%s", files[i].path ,files[i].name);
		if (files[i].type == 'p')
			printf ("\n- .cpp -> %s/%s", files[i].path ,files[i].name);
	}

	printf ("\n++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
	
	printf ("\nOtros:");
	for (int i = 0; i < n_files; i++)
	{
		if (files[i].type == 'x')
			printf ("\n-- xx --> %s/%s", files[i].path ,files[i].name);
	}

	for (int i = 0; i < n_routes; i++)
	{
		printf ("\n--> %s + %s", routes[i].alias, routes[i].path);
	}


	create_make (path, routes, &n_routes, files, &n_files);

	printf ("\n");
}
