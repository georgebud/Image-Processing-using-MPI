/*
 * TEMA3
 * Algoritmi Paraleli si Distribuiti
 * Budau George 332CC
 *
 * */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define TAG 0

typedef unsigned char u_char;

char TYPE[3];
int WIDTH, HEIGHT;
int MAX_VAL;

typedef struct {
    u_char value;
} BW;

typedef struct {
    u_char Red;
    u_char Green;
    u_char Blue;
} RGB;

typedef struct {
    RGB **rgb;
    BW **bw;
} IMG;

typedef struct {
    float smooth[3][3];
    float blur[3][3];
    float sharpen[3][3];
    float mean[3][3];
    float emboss[3][3];
} FILTER;

void initImage(char TYPE[3], IMG *image) {
    if (!strcmp(TYPE, "P5")) {
        image->bw = (BW **) calloc (HEIGHT, sizeof(BW*));
        for (int i = 0; i < HEIGHT; i++) {
            image->bw[i] = (BW *) calloc (WIDTH, sizeof(BW));
        }
    }
    else if (!strcmp(TYPE, "P6")) {
        image->rgb = (RGB **) calloc (HEIGHT, sizeof(RGB*));
        for (int i = 0; i < HEIGHT; i++) {
            image->rgb[i] = (RGB *) calloc (WIDTH, sizeof(RGB));
        }
    }
}

void readFile(char *inputFile, IMG *img) {
    FILE *file = fopen(inputFile, "rb");

    fscanf(file, "%s", TYPE);

    fseek(file, 1, SEEK_CUR); //pass over '\n'
    fscanf(file, "%*[^\n]"); //pass over entire line

    fscanf(file, "%d", &WIDTH);
    fscanf(file, "%d", &HEIGHT);
    fscanf(file, "%d", &MAX_VAL);

    fseek(file, 1, SEEK_CUR); //pass over '\n'
    if (!strcmp(TYPE, "P5")) {
        initImage("P5", img);
        for (int i = 0; i < HEIGHT; i++) 
            fread(img->bw[i], sizeof(BW), WIDTH, file); 
    }
    else if (!strcmp(TYPE, "P6")) {
        initImage("P6", img);
        for (int i = 0; i < HEIGHT; i++)
            fread(img->rgb[i], sizeof(RGB), WIDTH, file);
    }

    fclose(file);
}

void writeFile(char *outputFile, IMG *img) {
    FILE *file = fopen(outputFile, "wb");

    fprintf(file, "%s\n", TYPE);
    fprintf(file, "%d %d\n", WIDTH, HEIGHT);
    fprintf(file, "%d\n", MAX_VAL);

    if(!strcmp(TYPE, "P5")) {
        for (int i = 0; i < HEIGHT; i++)
            fwrite(img->bw[i], sizeof(BW), WIDTH, file);
    }

    if(!strcmp(TYPE, "P6")) {
        for (int i = 0; i < HEIGHT; i++)
            fwrite(img->rgb[i], sizeof(RGB), WIDTH, file);
    }

    fclose(file);
}

void initFilters(FILTER *filter) {
    FILTER f =
        {
            /*smooth*/
            {{1 / 9.0f, 1 / 9.0f, 1 / 9.0f},
            {1 / 9.0f, 1 / 9.0f, 1 / 9.0f},
            {1 / 9.0f, 1 / 9.0f, 1 / 9.0f}},

            /*blur*/
            {{1 / 16.0f, 2 / 16.0f, 1 / 16.0f},
            {2 / 16.0f, 4 / 16.0f, 2 / 16.0f},
            {1 / 16.0f, 2 / 16.0f, 1 / 16.0f}},

            /*sharpen*/
            {{0, -2 / 3.0f, 0},
            {-2 / 3.0f, 11 / 3.0f, -2 / 3.0f},
            {0, -2 / 3.0f, 0}},

            /*mean*/
            {{-1.0f, -1.0f, -1.0f},
            {-1.0f, 9.0f, -1.0f},
            {-1.0f, -1.0f, -1.0f}},

            /*emboss*/
            {{0.0f, -1.0f, 0.0f},
            {0.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f}}
        };

    *filter = f;
}

void ProcessingBW (IMG img, int rank, int nProc, int lowLimit, int highLimit, float f[3][3], IMG *out) {
    float result;
    int prevRank = rank - 1;
    int nextRank = rank + 1;
    int new_highLimit = highLimit - 1;
    int new_lowLimit = lowLimit - 1;

    if (rank != nProc - 1) {
        MPI_Send (img.bw[new_highLimit], WIDTH, MPI_UNSIGNED_CHAR, nextRank, TAG, MPI_COMM_WORLD);
    }
    if (rank != 0) {
        MPI_Recv (img.bw[new_lowLimit], WIDTH, MPI_UNSIGNED_CHAR, prevRank, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Send (img.bw[lowLimit], WIDTH, MPI_UNSIGNED_CHAR, prevRank, TAG, MPI_COMM_WORLD);
    }
    if (rank != nProc - 1) {
        MPI_Recv (img.bw[highLimit], WIDTH, MPI_UNSIGNED_CHAR, nextRank, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    for (int i = lowLimit; i < highLimit; i++) {
        for (int j = 0; j < WIDTH; j++) {
            result = 0;

            for (int p = i - 1; p < i + 2; p++) {
                for (int q = j - 1; q < j + 2; q++) {
                    float elem = 0;

                    if (!(p < 0 || q < 0 || q >= WIDTH || p >= HEIGHT)) {
                        elem = f[p - i + 1][q - j + 1];

                        result = result + img.bw[p][q].value * elem;
                    }

                }
            }

            if (result < 0)
                result = 0;
            else if (result > 255)
                result = 255;

            out->bw[i][j].value = (u_char) result;
        }
    }
}

void ProcessingRGB (IMG img, int rank, int nProc, int lowLimit, int highLimit, float f[3][3], IMG *out) {
    float res_R, res_G, res_B, result;
    int prevRank = rank - 1;
    int nextRank = rank + 1;
    int new_highLimit = highLimit - 1;
    int new_lowLimit = lowLimit - 1;

    if (rank != nProc - 1) {
        MPI_Send (img.rgb[new_highLimit], 3 * WIDTH, MPI_UNSIGNED_CHAR, nextRank, TAG, MPI_COMM_WORLD);
    }
    if (rank != 0) {
        MPI_Recv (img.rgb[new_lowLimit], 3 * WIDTH, MPI_UNSIGNED_CHAR, prevRank, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Send (img.rgb[lowLimit], 3 * WIDTH, MPI_UNSIGNED_CHAR, prevRank, TAG, MPI_COMM_WORLD);
    }
    if (rank != nProc - 1) {
        MPI_Recv (img.rgb[highLimit], 3 * WIDTH, MPI_UNSIGNED_CHAR, nextRank, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    for (int i = lowLimit; i < highLimit; i++) {
        for (int j = 0; j < WIDTH; j++) {
            res_R = 0, res_G = 0, res_B = 0, result = 0;

            for (int p = i - 1; p < i + 2; p++) {
                for (int q = j - 1; q < j + 2; q++) {
                    float elem = 0;

                    if (!(p < 0 || q < 0 || q >= WIDTH || p >= HEIGHT)) {
                        elem = f[p - i + 1][q - j + 1];

                        result = img.rgb[p][q].Red * elem;
                        res_R = res_R + result;

                        result = img.rgb[p][q].Green * elem;
                        res_G = res_G + result;

                        result = img.rgb[p][q].Blue * elem;
                        res_B = res_B + result;
                    }

                }
            }

            if (res_R < 0) res_R = 0;
            if (res_R > 255) res_R = 255;

            if (res_G < 0) res_G = 0;
            if (res_G > 255) res_G = 255;

            if (res_B < 0) res_B = 0;
            if (res_B > 255) res_B = 255;

            out->rgb[i][j].Red = (u_char) res_R;
            out->rgb[i][j].Green = (u_char) res_G;
            out->rgb[i][j].Blue = (u_char) res_B;
        }
    }
}

void setImageLimits(int *limit, int *lowLimit, int *highLimit, int totalProc, int process) {
    *limit = ceil((float) (HEIGHT / totalProc));
    *lowLimit = (*limit) * process;
    if (process != totalProc - 1) {
        *highLimit = (*limit) * process + (*limit);
    }
    else *highLimit = HEIGHT;
}

void checkFilters(int filter_given, float f[3][3], FILTER filter, char **argv) {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (!strcmp(argv[filter_given], "smooth")) f[i][j] = filter.smooth[i][j];
            else if (!strcmp(argv[filter_given], "blur")) f[i][j] = filter.blur[i][j];
            else if (!strcmp(argv[filter_given], "sharpen")) f[i][j] = filter.sharpen[i][j];
            else if (!strcmp(argv[filter_given], "mean")) f[i][j] = filter.mean[i][j];
            else if (!strcmp(argv[filter_given], "emboss")) f[i][j] = filter.emboss[i][j];
        }
    }
}

void copyValuesToInput(char TYPE[3], IMG *imgIN, IMG *imgOUT) {
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            if (!strcmp(TYPE, "P5")) {
                imgIN->bw[i][j].value = imgOUT->bw[i][j].value;
            }
            else if (!strcmp(TYPE, "P6")) {
                imgIN->rgb[i][j].Red = imgOUT->rgb[i][j].Red;
                imgIN->rgb[i][j].Green = imgOUT->rgb[i][j].Green;
                imgIN->rgb[i][j].Blue = imgOUT->rgb[i][j].Blue;
            }
        }
    }
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    int rank, nProcess;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nProcess);

    IMG inputFile, outputFile;

    FILTER filter;
    initFilters(&filter);

    int limit, lowLimit, highLimit;
    float f[3][3];

    if (argc < 4) {
        printf("Usage: mpirun -np N ./homework image_in.pnm image_out.pnm filter1 filter2 ... filterX\n");
        exit(-1);
    }

    if (rank == 0) {
        readFile(argv[1], &inputFile);
        initImage(TYPE, &outputFile);

        for (int iProc = 1; iProc < nProcess; iProc++) {
            MPI_Send(&HEIGHT, 1, MPI_INT, iProc, TAG, MPI_COMM_WORLD);
            MPI_Send(&WIDTH, 1, MPI_INT, iProc, TAG, MPI_COMM_WORLD);
            MPI_Send(&TYPE, 3, MPI_CHAR, iProc, TAG, MPI_COMM_WORLD);
            MPI_Send(&MAX_VAL, 1, MPI_INT, iProc, TAG, MPI_COMM_WORLD);

            setImageLimits(&limit, &lowLimit, &highLimit, nProcess, iProc);

            for (int i = lowLimit; i < highLimit; i++) {
                if (!strcmp(TYPE, "P5")) {
                    MPI_Send(inputFile.bw[i], WIDTH, MPI_UNSIGNED_CHAR, iProc, TAG, MPI_COMM_WORLD);
                }
                else if (!strcmp(TYPE, "P6")) {
                    MPI_Send(inputFile.rgb[i], 3 * WIDTH, MPI_UNSIGNED_CHAR, iProc, TAG, MPI_COMM_WORLD);
                }
            }
        }
    }
    else {
        MPI_Recv(&HEIGHT, 1, MPI_INT, 0, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&WIDTH, 1, MPI_INT, 0, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&TYPE, 3, MPI_CHAR, 0, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&MAX_VAL, 1, MPI_INT, 0, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        setImageLimits(&limit, &lowLimit, &highLimit, nProcess, rank);

        initImage(TYPE, &inputFile);
        initImage(TYPE, &outputFile);

        for (int i = lowLimit; i < highLimit; i++) {
            if (!strcmp(TYPE, "P5")) {
                MPI_Recv(inputFile.bw[i], WIDTH, MPI_UNSIGNED_CHAR, 0, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
            else if (!strcmp(TYPE, "P6")) {
                MPI_Recv(inputFile.rgb[i], 3 * WIDTH, MPI_UNSIGNED_CHAR, 0, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
        }
    }

    for (int iFilter = 3; iFilter < argc; iFilter++) {
        checkFilters(iFilter, f, filter, argv);

        limit = ceil((float) (HEIGHT / nProcess));
        if (rank == 0)
            lowLimit = 0;
        else lowLimit = limit * rank;
        if (rank != nProcess - 1)
            highLimit = limit * rank + limit;
        else highLimit = HEIGHT;

        if (!strcmp(TYPE, "P5")) {
            ProcessingBW(inputFile, rank, nProcess, lowLimit, highLimit, f, &outputFile);
            copyValuesToInput(TYPE, &inputFile, &outputFile);
        }
        else if (!strcmp(TYPE, "P6")) {
            ProcessingRGB(inputFile, rank, nProcess, lowLimit, highLimit, f, &outputFile);
            copyValuesToInput(TYPE, &inputFile, &outputFile);
        }
    }

    if (rank == 0) {
        for (int iProc = 1; iProc < nProcess; iProc++) {
            setImageLimits(&limit, &lowLimit, &highLimit, nProcess, iProc);

            for (int i = lowLimit; i < highLimit; i++) {
                if (!strcmp(TYPE, "P5")) {
                    MPI_Recv(outputFile.bw[i], WIDTH, MPI_UNSIGNED_CHAR, iProc, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
                else if (!strcmp(TYPE, "P6")) {
                    MPI_Recv(outputFile.rgb[i], 3 * WIDTH, MPI_UNSIGNED_CHAR, iProc, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
            }
        }

        writeFile(argv[2], &outputFile);
    }
    else {
        setImageLimits(&limit, &lowLimit, &highLimit, nProcess, rank);

        for (int i = lowLimit; i < highLimit; i++) {
            if (!strcmp(TYPE, "P5")) {
                MPI_Send(outputFile.bw[i], WIDTH, MPI_UNSIGNED_CHAR, 0, TAG, MPI_COMM_WORLD);
            }
            else if (!strcmp(TYPE, "P6")) {
                MPI_Send(outputFile.rgb[i], 3 * WIDTH, MPI_UNSIGNED_CHAR, 0, TAG, MPI_COMM_WORLD);
            }
        }
    }

    MPI_Finalize();
    return 0;
}
