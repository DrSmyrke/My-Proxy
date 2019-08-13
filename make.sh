#!/bin/bash
cd "Proxy Server"
qmake -project
qmake src.pro
make