@0xf1d6785cf4212789;
struct DownloadFileDiskData {
  path @0 :Text;
  size @1 :Int64;
  downloadSize @2 :Int64;
  chunkNum @3 :UInt32;
  chunksCompleteFlag @4 :List(UInt8);
  url @5 :Text;
  status @6: UInt8;
  bPause @7: Bool;
}