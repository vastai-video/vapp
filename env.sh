#!/bin/bash
  
INSTALL_DIR=/opt/vastai/vaststream
export VASTAI_VAPP_PATH=${INSTALL_DIR}/lib/op/ext_op/video/
export LD_LIBRARY_PATH=${INSTALL_DIR}/lib:${LD_LIBRARY_PATH}

