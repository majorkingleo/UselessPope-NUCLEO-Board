#pragma once


namespace BSP {

/**
  Initialize H7TwoFace internal file system. use H7TwoFace:: function to access it. eg:

  auto file = H7TwoFace::open("testfile", std::ios_base::in | std::ios_base::out );
  if( file ) { 
    file->write("hello", 6);  
  }
*/
void init_internal_fs();

} // namespace BSP