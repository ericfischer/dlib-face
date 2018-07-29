all: encode shape_predictor_5_face_landmarks.dat.bz2 dlib_face_recognition_resnet_model_v1.dat.bz2

shape_predictor_5_face_landmarks.dat.bz2:
	curl -L -O http://dlib.net/files/shape_predictor_5_face_landmarks.dat.bz2

dlib_face_recognition_resnet_model_v1.dat.bz2:
	curl -L -O http://dlib.net/files/dlib_face_recognition_resnet_model_v1.dat.bz2

encode: encode.cpp
	c++ -std=c++14 -g -Wall -O3 -o encode $< -ldlib -llapack -lcblas -lpng -lz -lgif
