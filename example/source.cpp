#include <cstdio>
#include <cstring>
#include "../ddeface.h"
#include "include/authpack.h"

#include <Windows.h>

#include "include/opencv/cv.h"
#include "include/opencv/highgui.h"
using namespace cv;

float expression_data[46], rotation_data[4], failure_data[2], pupil_pos[2], landmarks[150];

float* g_global_tables = NULL;

bool faceinit(){
	int sz;
	FILE *f = fopen("../assets/v3.bin", "rb");
	if (!f) return false;
	fseek(f, 0, SEEK_END);
	sz = ftell(f);
	fseek(f, 0, SEEK_SET);
	float *g_global_tables = (float*)malloc(sz);
	fread(g_global_tables, 1, sz, f);
	fclose(f);
	
	dde_setup((void*)g_global_tables, g_auth_package, sizeof(g_auth_package));
	//dde_setup((void*)g_global_tables, NULL, 0);
	return true;
}

bool ddefaceExample(unsigned int* img_raw, int w, int h) {
	int is_valid = 0, rotate = 0;
	easydde_reset();  // hide this line when tracking in a video.
	for (int i = 0; i < 64; i++) {
		is_valid = easydde_run(img_raw, w, h, 0);
		if (is_valid > 0 && i >= 60) {
			rotate = i % 4;
			break;
		}
	}

	if (is_valid > 0) {
		easydde_get_data(expression_data, 46, (char*)("expression"));
		easydde_get_data(rotation_data, 4, (char*)("rotation"));
		easydde_get_data(failure_data, 2, (char*)("face_confirmation_failure_stress"));
		easydde_get_data(pupil_pos, 2, (char*)("pupil_pos"));
		easydde_get_data(landmarks, 150, (char*)("landmarks"));

		if (failure_data[0] > 10) {
			printf("Face result error, reset.\n");
			easydde_reset();
			return false;
		}
		else if (failure_data[0] > 2) {
			printf("Invalid face result.\n");
			return false;
		}

		//set pupil position
		expression_data[6] = expression_data[7] = pupil_pos[0];
		expression_data[10] = expression_data[11] = -pupil_pos[0];
		expression_data[12] = expression_data[13] = pupil_pos[1];
		expression_data[4] = expression_data[5] = -pupil_pos[1];

		printf("Rotate mode: %d\n", rotate);
		return true;
	}
	else {
		printf("Face not found.\n");
		return false;
	}
}

void RotationFromQuaternion(float *q, float *R) {
	float x = q[0], y = q[1], z = q[2], w = q[3];
	float xy = x*y;
	float yz = y*z;
	float zx = z*x;
	float x2 = x*x;
	float y2 = y*y;
	float z2 = z*z;
	float xw = x*w;
	float yw = y*w;
	float zw = z*w;
	R[0] = 1.f - 2.f*(y2 + z2);
	R[1] = 2.f*(xy - zw);
	R[2] = 2.f*(zx + yw);
	R[4] = 2.f*(xy + zw);
	R[5] = 1.f - 2.f*(x2 + z2);
	R[6] = 2.f*(yz - xw);
	R[8] = 2.f*(zx - yw);
	R[9] = 2.f*(yz + xw);
	R[10] = 1.f - 2.f*(x2 + y2);
	R[15] = 1.f;
}

void MatrixMulti(float &x, float &y, float &z, float *R) {
	float x0, y0, z0;
	z = -z;
	x0 = R[0] * x + R[1] * y + R[2] * z;
	y0 = R[4] * x + R[5] * y + R[6] * z;
	z0 = R[8] * x + R[9] * y + R[10] * z;
	x = x0;
	y = y0;
	z = z0;
}

int main() {
	printf("FaceUnity SDK Example.\n\n");
	int alive = 1;
	char modelname[2048], picname[2048], name[2048];
	FILE *fbs = NULL, *fobj = NULL, *fout = NULL;

	if (!faceinit()) {
		printf("Error: load data error, please make sure \"v3.bin\" is loaded correctly and provide certificate (from authpack.h) in 'dde_setup'.\nPress any key to quit.\n");
		getchar();
		return -1;
	}

	while (alive) {

		if (alive == 1) {
			printf("Please input model name(\"shape_0\",\"Man\",\"OldMan\"): ");
			scanf(" %s", modelname);

			strcpy(name, "model/"); strcat(name, modelname); strcat(name, ".bs");
			fbs = fopen(name, "rb");
			strcpy(name, "model/"); strcat(name, modelname); strcat(name, ".obj");
			fobj = fopen(name, "r");

			if (!fbs || !fobj) {
				printf("Error: cannot open model files.\n");
				alive = 1;
				continue;
			}
		}

		alive = 2;
		printf("Choose a picture: ");
		scanf(" %s", picname);

		strcpy(name, "pic/"); strcat(name, picname);
		IplImage *img = cvLoadImage(name, 3);
		if (!img) {
			printf("Error: cannot open image, please choose again .\n");
			alive = 2;
			continue;
		}

		unsigned int *imagedata = (unsigned int *)malloc(img->width*img->height*4);
		char tmp[4];
		int step = img->widthStep / sizeof(uchar);
		for (int i = 0; i<img->height; i++)
			for (int j = 0; j < img->width; j++) {
				int r = img->imageData[i*step + j * 3 + 2] & 0xff;
				int g = img->imageData[i*step + j * 3 + 1] & 0xff;
				int b = img->imageData[i*step + j * 3] & 0xff;
				int a = 0xff;

				imagedata[(i)*img->width + j] = (a << 24) + (b << 16) + (g << 8) + r;
			}

		printf("Run FaceUnity sdk\n ...\n");
		int run_times = 5, valid = 0;
		while (!valid && run_times) {
			valid = ddefaceExample(imagedata, img->width, img->height);
			run_times--;
		}
		if (!valid) {
			alive = 2;
			continue;
		}

		printf("Output picture\n ...(Press any key to continue)\n");
		int landmarks_size = (int)(img->width < img->height ? img->width : img->height) / 500;
		for (int i = 0; i < 75; i++) {
			int x = (int)landmarks[i << 1];
			int y = (int)landmarks[(i << 1) + 1];
			
			for (int p = -landmarks_size; p <= landmarks_size; p++) {
				for (int q = -landmarks_size; q <= landmarks_size; q++) {
					int x1 = x + p, y1 = y + q;
					if (x1 >= 0 && x1 < img->width && y1 >= 0 && y1 < img->height) {
						img->imageData[y1 * step + x1 * 3] = 0xff;
						img->imageData[y1 * step + x1 * 3 + 1] = 0x00;
						img->imageData[y1 * step + x1 * 3 + 2] = 0x00;
					}
				}
			}
		}
		cvNamedWindow("landmarks", CV_WINDOW_AUTOSIZE);
		cvShowImage("landmarks", img); 
		cvWaitKey(0);
		cvDestroyWindow("landmarks");
		cvSaveImage("saveImage.jpg", img);

		free(imagedata);
		imagedata = NULL;

		printf("Output model\n ...\n");
		strcpy(name, "model/"); strcat(name, modelname); strcat(name, "-output.obj");
		fout = fopen(name, "w");
		int pnum, vnum;
		float *param_data, version, *param_change;
		fread(&version, sizeof(float), 1, fbs);
		fread(&pnum, sizeof(int), 1, fbs);
		fread(&vnum, sizeof(int), 1, fbs);

		param_data = (float*)malloc(sizeof(float) * vnum * 3);
		param_change = (float*)malloc(sizeof(float) * vnum * 3);

		fread(param_data, sizeof(float), vnum * 3, fbs);

		for (int i = 1; i < pnum; i++) {
			fread(param_change, sizeof(float), vnum * 3, fbs);
			for (int j = 0; j < vnum * 3; j++) {
				param_data[j] += param_change[j] * expression_data[i - 1];
			}
		}

		float R[16], m[4];
		RotationFromQuaternion(rotation_data, R);

		for (int i = 0; i < vnum; i++) {
			MatrixMulti(param_data[i * 3], param_data[i * 3 + 1], param_data[i * 3 + 2], R);
		}

		int index = 0, head = 0;
		float pa, pb, pc;
		char line[1024], str[32], ch, face[5][64];

		while (!feof(fobj)) {
			fgets(line, 1024, fobj);
			sscanf(line, " %s", str);
			if (str[0] == 'v' && strlen(str) == 1) {
				if (index >= vnum * 3) {
					sscanf(line + 1, " %f %f %f", &pa, &pb, &pc);
					//Special adjustment for model
					if (!strcmp(modelname, "Man")) {
						pa = pa * 0.069982;
						pb = (pb - 109.965637) * 0.069982;
						pc = (pc - 4.265675) * 0.069982;
					}
					else if (!strcmp(modelname, "OldMan")) {
						pa = pa * 0.167268;
						pb = (pb + 0.35782) * 0.167268;
						pc = (pc + 0.62820) * 0.167268;
					}
					MatrixMulti(pa, pb, pc, R);
					fprintf(fout, "v %f %f %f\n", pa, pb, pc);
				}
				else {
					fprintf(fout, "v %f %f %f\n", param_data[index], param_data[index + 1], param_data[index + 2]);
				}
				index += 3;
			}
			else if (str[0] == 'f' && strlen(str) == 1) {
				fprintf(fout, "f");
				if (sscanf(line + 1, " %s %s %s %s", face[0], face[1], face[2], face[3]) == 4) {
					fprintf(fout, " %s %s %s %s\n", face[3], face[2], face[1], face[0]);
				}
				else {
					sscanf(line + 1, " %s %s %s", face[0], face[1], face[2]);
					if (!strcmp(modelname, "shape_0")) fprintf(fout, " %s %s %s\n", face[0], face[1], face[2]);
					else fprintf(fout, " %s %s %s\n", face[2], face[1], face[0]);
				}
			}
			else {
				fprintf(fout, "%s", line);
			}
		}

		free(param_data); param_data = NULL;
		free(param_change); param_change = NULL;


		printf("Do you want to quit(Y or N)? ");
		scanf(" %c", &name[0]);
		if (name[0] == 'y' || name[0] == 'Y') { break; }
		else { alive = 1; }
	}

	fclose(fbs);
	fclose(fobj);
	fclose(fout);
	return 0;
}
