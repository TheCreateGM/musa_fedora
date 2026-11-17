/***********************************************************************************************************************
@File           smbctrldef.h
@Title          Contains the data structures used to control SMB
@Copyright      Copyright (c) 2025 Moore Threads Ltd. All Rights Reserved
@Description    Contains structures for SMB control 
@License        Strictly Confidential
***********************************************************************************************************************/
#ifndef SMB_CTRL_H_INCLUDED
#define SMB_CTRL_H_INCLUDED

#include "vputypes.h"

#define CODA9_AVC_Q_MATRIX_OFFSET       (0x3500)
#define CODA9_AVC_DIRECT_MEM_OFFSET     (0x10*2)
#define CODA9_MP4_INDIRECT_MEM_OFFSET   (0x330*2)
#define CODA9_MP4_DIRECT_MEM_OFFSET     (0x8*2)
#define CODA9_MP2_INDIRECT_MEM_OFFSET   (0x400*2)
#define CODA9_MP2_DIRECT_MEM_OFFSET     (0x24*2)
#define CODA9_VC1_DIRECT_MEM_OFFSET     (0x10*2)
#define CODA9_AVS_DIRECT_MEM_OFFSET     (0x10*2)
#define CODA9_AVS_INDIRECT_MEM_OFFSET   (0x600*2)
#define CODA9_VP8_DIRECT_MEM_OFFSET     (0x8*2)

#pragma pack(push, 1)
typedef struct {
//------------------------------------------------------------------------------
//    SPS/PPS/SLICE HEADER VARIABLES
//------------------------------------------------------------------------------
    Uint16 dSeqParaSetId;           //10h
    Uint16 dProfileIdc;             //11h
    Uint16 dMaxRefFrame;            //12h
    Uint16 dMaxFrameBit;            //13h
    Uint16 dMaxFrameNumH;           //14h
    Uint16 dMaxFrameNumL;           //15h
    Uint16 dPicOrderCntType;        //16h
    Uint16 dPicOrderCntBit;         //17h
    Uint16 dPicOrderZeroFlag;       //18h
    Uint16 dNumRefFrame;            //19h
    Uint16 dGapsInFrameNum;         //1Ah
    Uint16 dFrameMbsOnlyFlag;       //1Bh
    Uint16 dMbAffFlag;              //1Ch
    Uint16 dDirect8x8Flag;          //1Dh
    Uint16 dMbAddrH;                //1Eh
    Uint16 dLevelIdc;               //1Fh
    Uint16 dMaxDpbSize;             //20h
    Uint16 dReorderEnable;          //21h
    Uint16 dChromaIdc;              //22h
    Uint16 dPicStructFlag;          //23h
    Uint16 dConstSetflag7_0;        //24h
    Uint16 dAspectRatioH;           //25h
    Uint16 dAspectRatioL;           //26h
    Uint16 dBitRateH;               //27h
    Uint16 dBitRateL;               //28h
    Uint16 dPicParaSetId;           //29h
    Uint16 dPicOrderPrsFlag;        //2Ah
    Uint16 dNumSliceGrpMinus1;      //2Bh
    Uint16 dPicNumRefIdxL0Minus1;   //2Ch
    Uint16 dUseCabac;               //2Dh
    Uint16 dPicNumRefIdxL1Minus1;   //2Eh
    Uint16 dWpFlag;                 //2Fh
    Uint16 dWpIdc;                  //30h
    Uint16 dPicInitQpY;             //31h
    Uint16 dChromaQpOffset;         //32h
    Uint16 dDeblkCtrlPresent;       //33h
    Uint16 dConstrainIntra;         //34h
    Uint16 dRedundPicCntPresent;    //35h
    Uint16 dSecondChromaQpOffset;   //36h
    Uint16 dTransform8x8ModeFlag;   //37h
    Uint16 dSliceType;              //38h
    Uint16 dFrameNum;               //39h
    Uint16 dFirstFieldExist;        //3Ah
    Uint16 dFirstFieldNalRefFlag;   //3Bh
    Uint16 dFirstFieldFrameNum;     //3Ch
    Uint16 dFirstFieldBottom;       //3Dh
    Uint16 dFirstFieldFrameIdx;     //3Eh
    Uint16 dFieldPicFlag;           //3Fh
    Uint16 dSecondField;            //40h
    Uint16 dPicType;                //41h
    Uint16 dBottomFieldFlag;        //42h
    Uint16 dMbAff;                  //43h
    Uint16 dMbNumPicL;              //44h
    Uint16 dIdrPicId;               //45h
    Uint16 dDirectMvMode;           //46h
    Uint16 dCabacInitIdc;           //47h
    Uint16 dNumRefIdxL0Minus1;      //48h
    Uint16 dNumRefIdxL1Minus1;      //49h
    Uint16 dSliceQpY;               //4Ah
    Uint16 dDisableDeblk;           //4Bh
    Uint16 dDeblkOffsetA;           //4Ch
    Uint16 dDeblkOffsetB;           //4Dh

//------------------------------------------------------------------------------
//    PICTURE VARIABLES
//------------------------------------------------------------------------------
    Uint16 dNalRefIdc;              //4Eh
    Uint16 dIdrPicture;             //4Fh
    Uint16 dCurFrameNum;            //50h
    Uint16 dFirstSlice;             //51h
    Uint16 dMbNumPicH;              //52h
    Uint16 dDummy;                  //53h
    Uint16 dDecFrameNumH;           //54h
    Uint16 dDecFrameNumL;           //55h
    Uint16 dPrevDispPicIdx;         //56h

//------------------------------------------------------------------------------
//    MB VARIABLES
//------------------------------------------------------------------------------
    Uint16 dMbNumX;                 //57h
    Uint16 dMbNumY;                 //58h
    Uint16 dMbAddrL;                //59h
    Uint16 dMbIntra;                //5Ah
    Uint16 dMbPosX;                 //5Bh
    Uint16 dMbPosY;                 //5Ch
    Uint16 dMbType;                 //5Dh
    Uint16 dMcWpMode;               //5Eh
    Uint16 dMcLogWdYc;              //5Fh
    Uint16 dBIdxCur;                //60h
    Uint16 dCpbMinus1;              //61h
    Uint16 dHeaderIsrFlag;          //62h
    Uint16 dBIdxNxt;                //63h
    Uint16 dBIdxSaveR7;             //64h
    Uint16 dPsRetFlag;              //65h
    Uint16 dPicStride;              //66h

//------------------------------------------------------------------------------
//    HP SCALING_MATRIX_PARAMETER
//------------------------------------------------------------------------------
    Uint16 dSeqScalingMatrixFlag;   //67h
    Uint16 dPicScalingMatrixFlag;   //68h
    Uint16 dSeqScalingListFlag;     //69h
    Uint16 dPicScalingListFlag;     //6Ah
    Uint16 dSeqDefScalingList;      //6Bh
    Uint16 dPicDefScalingList;      //6Ch

//------------------------------------------------------------------------------
//    MV COL
//------------------------------------------------------------------------------
    Uint16 dReorderInitListNum;     //6Dh
    Uint16 dDecColPicAddrH;         //6Eh
    Uint16 dDecColPicAddrL;         //6Fh
    Uint16 dRefColPicAddrH;         //70h
    Uint16 dRefColPicAddrL;         //71h
    Uint16 dDpbPocCurH;             //72h
    Uint16 dDpbPocCurL;             //73h
    Uint16 dDpbPocTopH;             //74h
    Uint16 dDpbPocTopL;             //75h
    Uint16 dDpbPocBotH;             //76h
    Uint16 dDpbPocBotL;             //77h
    Uint16 dColPicLong;             //78h
    Uint16 dColPicField;            //79h
    Uint16 dNearPocBot;             //7Ah

//------------------------------------------------------------------------------
//    MMCO
//------------------------------------------------------------------------------
    Uint16 dMmcoCurRefNum;          //7Bh
    Uint16 dMmcoCurRefType;         //7Ch
    Uint16 dMmcoSlideWinOutFlag;    //7Dh
    Uint16 dMmcoResetFlag;          //7Eh
    Uint16 dFrameBufNum;            //7Fh
    Uint16 dMmcoPrevRefFrameNum;    //80h
    Uint16 dMmcoPrevDispPicIdx;     //81h
    Uint16 dDpbPrevMmcoResetFlag;   //82h

//------------------------------------------------------------------------------
//    FrameIdx
//------------------------------------------------------------------------------
    Uint16 dPrevDecFrameIdx;        //83h
    Uint16 dDecFrameIdx;            //84h
    Uint16 dDispFrameIdx;           //85h
    Uint16 dSuccess;                //86h

//------------------------------------------------------------------------------
//    ROTATOR (POST PROCESSING)
//------------------------------------------------------------------------------
    Uint16 dPostRotateMode;         //87h
    Uint16 dPostRotateEn;           //88h
    Uint16 dPostInFrmIdx;           //89h
    Uint16 dPostDstFrmIdx;          //8Ah
    Uint16 dPostMbNumX;             //8Bh
    Uint16 dPostMbNumY;             //8Ch
    Uint16 dPostMbPosX;             //8Dh
    Uint16 dPostMbPosY;             //8Eh
    Uint16 dPostProcEn;             //8Fh

//------------------------------------------------------------------------------
//    SLICE BUFFER / VUI PARAMETER
//------------------------------------------------------------------------------
    Uint16 dNalHrdParamFlag;        //90h
    Uint16 dVclHrdParamFlag;        //91h
    Uint16 dPrevNalRefIdc;          //92h
    Uint16 dPrevIdrPicture;         //93h
    Uint16 dFirstVclNal;            //94h
    Uint16 dSaveNalRefIdc;          //95h
    Uint16 dSaveIdrPicture;         //96h
    Uint16 dBpSliceBufLack;         //97h

//------------------------------------------------------------------------------
//    SPS/PPS BUFFER
//------------------------------------------------------------------------------
    Uint16 dPpsBackupedFlag;        //98h
    Uint16 dSpsBufRdPtrH;           //99h
    Uint16 dSpsBufRdPtrL;           //9Ah
    Uint16 dPsBufIsrFlag;           //9Bh
    Uint16 dAlignBit;               //9Ch
    Uint16 dZeroNum;                //9Dh
    Uint16 dNumBit;                 //9Eh
    Uint16 dMcFastInterpolDisable;  //9Fh

//------------------------------------------------------------------------------
//    ADD-ON FEATURE VARIABLES
//------------------------------------------------------------------------------
    Uint16 dIdrPicFlagField;        //A0h
    Uint16 dSliceTypeDecoded;       //A1h
    Uint16 dSliceTypeField;         //A2h
    Uint16 dPicStruct;              //A3h
    Uint16 dNpfIdx;                 //A4h
    Uint16 dSkipFlag;               //A5h
    Uint16 dUserDataPtr;            //A6h
    Uint16 dMvcReportH;             //A7h
    Uint16 dMvcReportL;             //A8h
    Uint16 dBackupPsId;             //A9h
    Uint16 dDisableConcealMethods;  //AAh
    Uint16 dAvcDecCodeNumH;         //ABh
    Uint16 dAvcDecCodeNumL;         //ACh
    Uint16 dAuFound;                //ADh
    Uint16 dErrReasonFlag;          //AEh
    Uint16 dEnUserFrmDelay;         //AFh
    Uint16 dActiveFormat;           //B0h
    Uint16 dX264MvExpEn;            //B1h
    Uint16 dFrameNumbak;            //B2h
    Uint16 dDpbBufferingReported;   //B3h
    Uint16 dChunkReuse;             //B4h
    Uint16 dUserDataBaseAddrH;      //B5h
    Uint16 dUserDataBaseAddrL;      //B6h
    Uint16 dUserDataBufSizeH;       //B7h
    Uint16 dUserDataBufSizeL;       //B8h
    Uint16 dSliceTypeCur;           //B9h
    Uint16 dSliceTypeTop;           //BAh
    Uint16 dSliceTypeBot;           //BBh
    Uint16 dIdrFrameFlag;           //BCh
    Uint16 dSeqChanged;             //BDh
    Uint16 dWrongFrameNumFlag;      //BEh
    Uint16 dClosedCaptionType;      //BFh
    Uint16 dRecoveryFrameCnt;       //C0h
    Uint16 dExactRecoveryPoint;     //C1h
    Uint16 dBrokenLinkFlag;         //C2h
    Uint16 dChangingSliceGroupIdc;  //C3h
    Uint16 dDpbAllocFlag;           //C4h
    Uint16 dDpbPutPicFlag;          //C5h
    Uint16 dMbNumInPicL;            //C6h
    Uint16 dMbNumInPicH;            //C7h
    Uint16 dFirstMbAddrL;           //C8h
    Uint16 dFirstMbAddrH;           //C9h
    Uint16 dFrmErrMbNumL;           //CAh
    Uint16 dFrmErrMbNumH;           //CBh
    Uint16 FREE_CC;                 //CCh
    Uint16 FREE_CD;                 //CDh
    Uint16 FREE_CE;                 //CEh
    Uint16 FREE_CF;                 //CFh
    Uint16 dMbSkipRun;              //D0h
    Uint16 dSkipRunCntDown;         //D1h
    Uint16 FREE_D2;                 //D2h
    Uint16 dFirstFrame;             //D3h

//------------------------------------------------------------------------------
//    MULTI VPU FULL FRAME PARALLEL DECODER
//------------------------------------------------------------------------------
    Uint16 dMbySyncEnable;          //D4h
    Uint16 dMbySyncFrameNum;        //D5h

//------------------------------------------------------------------------------
//    AVC_BASELINE
//------------------------------------------------------------------------------
    Uint16 dFmoMapType;             //D6h
    Uint16 dFmoNumGrp;              //D7h
    Uint16 dFmoNumGrpMinus1;        //D8h
    Uint16 dFmoGrp;                 //D9h
    Uint16 dFmoGrpChangeDir;        //DAh
    Uint16 dFmoGrpChangeRate;       //DBh
    Uint16 dFmoMbNumGrp0;           //DCh
    Uint16 dFmoMap345;              //DDh
    Uint16 dFmoCycleBit;            //DEh
    Uint16 dFmoSliceBufInit;        //DFh
    Uint16 dFmoPrevGrp;             //E0h
    Uint16 dFmoMbAvailA;            //E1h
    Uint16 dFmoMbAvailD;            //E2h
    Uint16 dFmoMbAvail;             //E3h
    Uint16 dExtBitBufR1Bak;         //E4h

//------------------------------------------------------------------------------
//    DPB
//------------------------------------------------------------------------------
    Uint16 dDpbBufCnt;              //E5h
    Uint16 dDpbDispFifoWrPtr;       //E6h
    Uint16 dDpbDispFifoRdPtr;       //E7h
    Uint16 dDpbDispFifoCnt;         //E8h
    Uint16 dDpbFrameDelay;          //E9h
    Uint16 dDpbNumReorderFrame;     //EAh
    Uint16 dDpbMaxDecBuffering;     //EBh
} Coda9AvcDirectMemory;

typedef struct {
    Uint16 dChromaIdc;              //08h
    Uint16 dPicX;                   //09h
    Uint16 dPicY;                   //0Ah
    Uint16 dFieldPrediction;        //0Bh
    Uint16 dVopFcodeForward;        //0Ch
    Uint16 dVopFcodeBackward;       //0Dh
    Uint16 dQuantType;              //0Eh
    Uint16 dPackModeBak;            //0Fh
    Uint16 dPackModeOp;             //10h
    Uint16 dFCodeResBitForward;     //11h
    Uint16 dFCodeResMaskForward;    //12h
    Uint16 dFCodeResBitBackward;    //13h
    Uint16 dFCodeResMaskBackward;   //14h
    Uint16 dPackMode;               //15h
    Uint16 dUserDivx;               //16h
    Uint16 dPrevVop;                //17h
    Uint16 dQpel;                   //18h
    Uint16 dMp4LastDisp;            //19h
    Uint16 dCheckReuse;             //1Ah
    Uint16 dDecFrmIdx;              //1Bh
    Uint16 dDispFrmIdx;             //1Ch
    Uint16 dFrameSkipMode;          //1Dh
    Uint16 dIframeSch;              //1Eh
    Uint16 dSuccess;                //1Fh

//------------------------------------------------------------------------------
//    [20 - 3F] FRAME VARIABLES
//------------------------------------------------------------------------------
    Uint16 dVopTimeRes;             //20h
    Uint16 dVopTimeInc;             //21h
    Uint16 dDataPartEn;             //22h
    Uint16 dRVlcEn;                 //23h
    Uint16 dVopType;                //24h
    Uint16 dRoundType;              //25h
    Uint16 dIntraDcVlcThr;          //26h
    Uint16 dVopQuant;               //27h
    Uint16 dFCode;                  //28h
    Uint16 dFCodeResBit;            //29h
    Uint16 dFCodeResMask;           //2Ah
    Uint16 dVopTime;                //2Bh
    Uint16 dReduceResEn;            //2Ch
    Uint16 dVopCoded;               //2Dh
    Uint16 dFlv263;                 //2Eh
    Uint16 dFlvFormat;              //2Fh
    Uint16 dShortVideoHeader;       //30h
    Uint16 dNumGob;                 //31h
    Uint16 dMbNumGob;               //32h
    Uint16 dGobNum;                 //33h
    Uint16 dAnnexI;                 //34h
    Uint16 dAnnexJ;                 //35h
    Uint16 dAnnexK;                 //36h
    Uint16 dAnnexT;                 //37h
    Uint16 dCustomPcf;              //38h
    Uint16 dAnnexD;                 //39h
    Uint16 dPlusType;               //3Ah
    Uint16 dUui;                    //3Bh
    Uint16 dVolDecoded;             //3Ch
    Uint16 dTimeIncrBits;           //3Dh
    Uint16 dBackRefI;               //3Eh
    Uint16 dSpriteVop;              //3Fh

//------------------------------------------------------------------------------
//    [40 - 4F] VARIABLES
//------------------------------------------------------------------------------
    Uint16 dGobFrameId;             //40h
    Uint16 dGobNumDec;              //41h
    Uint16 dConcealMbQs;            //42h
    Uint16 dFirstMbAddr;            //43h
    Uint16 dDecMbAddr;              //44h
    Uint16 dEndMbAddr;              //45h
    Uint16 dErrMbNum;               //46h
    Uint16 dNumPad;                 //47h
    Uint16 dNumBit;                 //48h
    Uint16 dTempBit;                //49h
    Uint16 dMp4DecFrmNumH;          //4Ah
    Uint16 dMp4DecFrmNumL;          //4Bh
    Uint16 dMbOffset;               //4Ch
    Uint16 dFrameBufNum;            //4Dh
    Uint16 dVopTimeAsp;             //4Eh
    Uint16 dMbDpErrFlag;            //4Fh

//------------------------------------------------------------------------------
//    [50 - 5F] MB VARIABLES
//------------------------------------------------------------------------------
    Uint16 dMbNumX;                 //50h
    Uint16 dMbNumY;                 //51h
    Uint16 dMbNumInPic;             //52h
    Uint16 dMbAddr;                 //53h
    Uint16 dMbPosX;                 //54h
    Uint16 dMbPosY;                 //55h
    Uint16 dMbModb;                 //56h
    Uint16 dDummy1;                 //57h
    Uint16 dMbIntra;                //58h
    Uint16 dNotCoded;               //59h
    Uint16 dMbTypeQ;                //5Ah
    Uint16 dMbType4V;               //5Bh
    Uint16 dAcPredFlag;             //5Ch
    Uint16 dMbQuant;                //5Dh
    Uint16 dCbp;                    //5Eh
    Uint16 dUseIntraDcVlc;          //5Fh

//------------------------------------------------------------------------------
//    [60 - 6F] VARIABLES
//------------------------------------------------------------------------------
    Uint16 dReSyncMarkBit;          //60h
    Uint16 dPrevMbQuant;            //61h
    Uint16 dPicStride;              //62h
    Uint16 dMp4DecDbkOn;            //63h
    Uint16 dForPred;                //64h
    Uint16 dBackPred;               //65h
    Uint16 dBVopFlag;               //66h
    Uint16 dDpBufPage;              //67h
    Uint16 dDpBufPtr;               //68h
    Uint16 dTRD;                    //69h
    Uint16 dTRB;                    //6Ah
    Uint16 dSliceId;                //6Bh
    Uint16 dBMbType;                //6Ch
    Uint16 dRefCnt;                 //6Dh
    Uint16 dDummy2;                 //6Eh
    Uint16 dDummy3;                 //6Fh
    Uint16 dFirstPkt;               //70h
    Uint16 dDeringEn;               //71h
    Uint16 dQpLoadBuf;              //72h
    Uint16 dQpSaveBuf;              //73h
    Uint16 dPrevBVopFlag;           //74h
    Uint16 dRotEn;                  //75h
    Uint16 dRotMode;                //76h
    Uint16 dRotProcEn;              //77h
    Uint16 dRotLoadEn;              //78h
    Uint16 dQpProc;                 //79h
    Uint16 dParCode;                //7Ah

//-----------------------------------------------------------------------------
//    [7B - 7F] Padding
//-----------------------------------------------------------------------------
    Uint16 dEosPattern;             //7Bh
    Uint16 dPadTmp0;                //7Ch
    Uint16 dPadTmp1;                //7Dh

//-----------------------------------------------------------------------------
//    [7B - 9F] BitStream
//-----------------------------------------------------------------------------
    Uint16 dDummy4;                 //7Eh
    Uint16 dDummy5;                 //7Fh
    Uint16 dExtBBStaAddrH;          //80h
    Uint16 dExtBBStaAddrL;          //81h
    Uint16 dExtBBSizeH;             //82h
    Uint16 dExtBBSizeL;             //83h
    Uint16 dExtBBEndAddrH;          //84h
    Uint16 dExtBBEndAddrL;          //85h
    Uint16 dExtBBRdPtrH;            //86h
    Uint16 dExtBBRdPtrL;            //87h
    Uint16 dExtBBWrPtrH;            //88h
    Uint16 dExtBBWrPtrL;            //89h
    Uint16 dExtStartOffset;         //8Ah
    Uint16 dExtBitBufIntFlag;       //8Bh
    Uint16 dExtBitBufEndFlag;       //8Ch
    Uint16 dGbuBufMemSel;           //8Dh
    Uint16 dStreamResycDone;        //8Eh
    Uint16 dDoubleBufEn;            //8Fh
    Uint16 dExtResidue;             //90h
    Uint16 dPicEndMode;             //91h
    Uint16 dSignExtH;               //92h
    Uint16 dSignExtL;               //93h
    Uint16 dNalUnitType;            //94h
    Uint16 dRbspEndFlag;            //95h
    Uint16 dRbspEndSize;            //96h
    Uint16 dRbspEndByteCntL;        //97h
    Uint16 dRbspEndByteCntH;        //98h
    Uint16 dBitBufRegBir;           //99h
    Uint16 dBitBufPreLoad;          //9Ah
    Uint16 dExtBBResidue;           //9Bh
    Uint16 dTcntH;                  //9Ch
    Uint16 dTcntL;                  //9Dh
    Uint16 dTcntBackupH;            //9Eh
    Uint16 dTcntBackupL;            //9Fh

//------------------------------------------------------------------------------
//    [A0 - AF] RBSP NAL
//------------------------------------------------------------------------------
    Uint16 dFwdRefIdx;              //A0h
    Uint16 dBwdRefIdx;              //A1h
    Uint16 dRecIdx;                 //A2h
    Uint16 dIPDelayIdx;             //A3h
    Uint16 dPrevDispIdx;            //A4h
    Uint16 dOutputIdx;              //A5h
    Uint16 dDbkIdx;                 //A6h
    Uint16 dPrevFrmBufIdx;          //A7h
    Uint16 dFwdRefTimeBase;         //A8h
    Uint16 dBwdRefTimeBase;         //A9h
    Uint16 dUseDownQMat;            //AAh
    Uint16 dDctType;                //ABh
    Uint16 dAlterVertFlag;          //ACh
    Uint16 dInterlaced;             //ADh
    Uint16 dTopFieldFirst;          //AEh
    Uint16 dFwdRefVopTime;          //AFh

//------------------------------------------------------------------------------
//    [B0 - BF] B-VOP
//------------------------------------------------------------------------------
    Uint16 dBwdRefVopTime;          //B0h
    Uint16 dFixedRate;              //B1h
    Uint16 dTFrame;                 //B2h
    Uint16 dFTRD;                   //B3h
    Uint16 dFTRB;                   //B4h
    Uint16 dFwTopFieldRef;          //B5h
    Uint16 dFwBotFieldRef;          //B6h
    Uint16 dBwTopFieldRef;          //B7h
    Uint16 dBwBotFieldRef;          //B8h
    Uint16 dFieldDelta;             //B9h
    Uint16 dConcealCnt;             //BAh
    Uint16 dMvpRunFlag;             //BBh
    Uint16 dAxiSecEnBlk;            //BCh
    Uint16 dAxiSecUseSramAddr;      //BDh
    Uint16 dDisposablePic;          //BEh
    Uint16 dPrevDisposablePic;      //BFh

//------------------------------------------------------------------------------
//    [C1 - C5] GMC
//------------------------------------------------------------------------------
    Uint16 dGmcDisable;             //C0h
    Uint16 dSpriteGmc;              //C1h
    Uint16 dNoOfWarpPoints;         //C2h
    Uint16 dGmcBrightChange;        //C3h
    Uint16 dWarpAccuracy;           //C4h
    Uint16 dMcSel;                  //C5h
    Uint16 reserved0;
    Uint16 reserved1;
    /* size need alignment 8 at least */
} Coda9Mp4DirectMemory;

typedef struct {
    Uint16 tmp0;
    Uint16 tmp1;
    Uint16 pDownQMatIntra[64];      //332h
    Uint16 pDownQMatInter[64];      //372h
    Uint16 pTraj[2];                //3B2h
} Coda9Mp4IndirectMemory;

typedef struct {
//------------------------------------------------------------------------------
//    [20 - 2F] STD CODEC/ORDERING/DEBUG LAYER SUBROUTINE
//------------------------------------------------------------------------------
    Uint16 tmp0;
    Uint16 dMpeg2;                  //025h
    Uint16 dProgSeq;                //026h
    Uint16 dLoadBit;                //027h
    Uint16 dFrameRate;              //028h
    Uint16 dFRateExtN;              //029h
    Uint16 dFRateExtD;              //02Ah
    Uint16 dRefCnt;                 //02Bh
    Uint16 dProfile;                //02Ch
    Uint16 dLevel;                  //02Dh
    Uint16 dDummy0;                 //02Eh

//------------------------------------------------------------------------------
//    [30 - 4F] PICTURE LAYER SUBROUTINE
//------------------------------------------------------------------------------
    Uint16 dPicTypePrev;            //02Fh
    Uint16 dDcPrecision;            //030h
    Uint16 dFramePic;               //031h
    Uint16 dFieldPic;               //032h
    Uint16 dField;                  //033h
    Uint16 dPicType;                //034h
    Uint16 dAlternateScan;          //035h
    Uint16 dQScaleType;             //036h
    Uint16 dVlcFormat;              //037h
    Uint16 dConcealMV;              //038h
    Uint16 dTopFieldFirst;          //039h
    Uint16 dRepeatFirst;            //03Ah
    Uint16 dSecondField;            //03Bh
    Uint16 dFrameDct;               //03Ch
    Uint16 dPicStruct;              //03Dh
    Uint16 dProgFrame;              //03Eh
    Uint16 dFieldSeq;               //03Fh
    Uint16 dPictureI;               //040h
    Uint16 dPictureP;               //041h
    Uint16 dPictureB;               //042h
    Uint16 dFirstFieldPicType;      //043h
    Uint16 dFirstFieldPictureI;     //044h
    Uint16 dMbFieldY;               //045h
    Uint16 dVertMBA;                //046h
    Uint16 dFirstMbInSlice;         //047h
    Uint16 dMbAInc;                 //048h
    Uint16 dPictureD;               //049h
    Uint16 dMp2MvpCmd1;             //04Ah
    Uint16 dMp2MvpCmd2;             //04Bh
    Uint16 dLinearFrmIdx;           //04Ch
    Uint16 dDummy1;                 //04Dh
    Uint16 dDummy2;                 //04Eh
    Uint16 dErrReasonFlag;          //04Fh
    Uint16 dTemp;                   //050h
    Uint16 dTemp2;                  //051h
    Uint16 dPicX;                   //052h
    Uint16 dPicY;                   //053h
    Uint16 dMbNumInPic;             //054h
    Uint16 dMbNumX;                 //055h
    Uint16 dMbNumY;                 //056h
    Uint16 dMbAddr;                 //057h
    Uint16 dMbPosX;                 //058h
    Uint16 dMbPosY;                 //059h
    Uint16 dNumPad;                 //05Ah
    Uint16 dActiveFormat;           //05Bh
    Uint16 dPrevVertMBA;            //05Ch
    Uint16 dPicXSeq;                //05Dh
    Uint16 dPicYSeq;                //05Eh
    Uint16 dPicHeaderDecoded;       //05Fh
} Coda9Mp2DirectMemory;

typedef struct {
    Uint16 pRSize[2][2];            //400h
    Uint16 pLimit[2][2];            //404h
    Uint16 pDummy[8];               //408h
    Uint16 pIdxQueue[16];           //410h
    Uint16 pIntraQMat[64];          //420h
    Uint16 pInterQMat[64];          //460h
} Coda9Mp2IndirectMemory;

typedef struct {
    Uint16 dSeqPicWidth;            //08h
    Uint16 dSeqPicHeight;           //09h
    Uint16 dSeqHScale;              //0Ah
    Uint16 dSeqVScale;              //0Bh
    Uint16 dMbNumX;                 //0Ch
    Uint16 dMbNumY;                 //0Dh
    Uint16 dMbNumInPic;             //0Eh
    Uint16 dPicChunkSizeH;          //0Fh
    Uint16 dPicChunkSizeL;          //10h
    Uint16 dPicHasSeqMeta;          //11h
    Uint16 dPicType;                //12h
    Uint16 dPicVersionNumber;       //13h
    Uint16 dPicShowFrame;           //14h
    Uint16 dPicFirstDpSizeH;        //15h
    Uint16 dPicFirstDpSizeL;        //16h
    Uint16 reserved;                //17h
} Coda9Vp8DirectMemory;

typedef struct {
    Uint16 dProfile;                //010h
    Uint16 dLevel;                  //011h
    Uint16 dChromaIdc;              //012h
    Uint16 dByteOffset;             //013h
    Uint16 dBitOffset;              //014h
    Uint16 dPostProcFlag;           //015h
    Uint16 dMaxCodedX;              //016h
    Uint16 dMaxCodedY;              //017h
    Uint16 dPullDown;               //018h
    Uint16 dInterlace;              //019h
    Uint16 dTfcntrFlag;             //01Ah
    Uint16 dFInterpFlag;            //01Bh
    Uint16 dPSF;                    //01Ch
    Uint16 dDisplayX;               //01Dh
    Uint16 dDisplayY;               //01Eh
    Uint16 dAspectX;                //01Fh
    Uint16 dAspectY;                //020h
    Uint16 dFrameRateNr;            //021h
    Uint16 dFrameRateDr;            //022h
    Uint16 dColorPrim;              //023h
    Uint16 dTransferChar;           //024h
    Uint16 dMatrixCoef;             //025h
    Uint16 dHrdParamFlag;           //026h
    Uint16 dHrdNumLeakyBuckets;     //027h
    Uint16 dBrokenLink;             //028h
    Uint16 dClosedEntry;            //029h
    Uint16 dPanScanFlag;            //02Ah
    Uint16 dRefDistFlag;            //02Bh
    Uint16 dLoopFilter;             //02Ch
    Uint16 dFastUVMc;               //02Dh
    Uint16 dExtendedMV;             //02Eh
    Uint16 dDQuant;                 //02Fh
    Uint16 dVSTransform;            //030h
    Uint16 dOverlap;                //031h
    Uint16 dQuantizer;              //032h
    Uint16 dSeqCodedX;              //033h
    Uint16 dSeqCodedY;              //034h
    Uint16 dExtendedDMV;            //035h
    Uint16 dRangeMapYFlag;          //036h
    Uint16 dRangeMapY;              //037h
    Uint16 dRangeMapCFlag;          //038h
    Uint16 dRangeMapC;              //039h
    Uint16 dNumFrames;              //03Ah
    Uint16 dDummy0;                 //03Bh
    Uint16 dStreamFormat;           //03Ch
    Uint16 dMultiRes;               //03Dh
    Uint16 dSyncmarker;             //03Eh
    Uint16 dRangeRed;               //03Fh
    Uint16 dMaxBFrames;             //040h
    Uint16 dFrameSize;              //041h
    Uint16 dDummy1;                 //042h
    Uint16 dX8Intra;                //043h
    Uint16 dBit16Xform;             //044h
    Uint16 dDctTabSwitch;           //045h
    Uint16 dBetaRTM;                //046h
    Uint16 dNumberOfPanScanWindows; //047h
    Uint16 dYUV411;                 //048h
    Uint16 dSprite;                 //049h
    Uint16 dFrameMode;              //04Ah
    Uint16 dPicType0;               //04Bh
    Uint16 dPicType1;               //04Ch
    Uint16 dTempRefCount;           //04Dh
    Uint16 dTopFieldFirst;          //04Eh
    Uint16 dRptFirstField;          //04Fh
    Uint16 dRptFrameCount;          //050h
    Uint16 dSecondField;            //051h
    Uint16 dBottomField;            //052h
    Uint16 dCodedX;                 //053h
    Uint16 dCodedY;                 //054h
    Uint16 dMbNumX;                 //055h
    Uint16 dMbNumY;                 //056h
    Uint16 dSliceY;                 //057h
    Uint16 dUVSamp;                 //058h
    Uint16 dInterpFrm;              //059h
    Uint16 dPostProc;               //05Ah
    Uint16 dResPic;                 //05Bh
    Uint16 dRangeRedFrm;            //05Ch
    Uint16 dBFraction;              //05Dh
    Uint16 dRefDist;                //05Eh
    Uint16 dNumRef;                 //05Fh
    Uint16 dRefField;               //060h
    Uint16 dRndCtrl;                //061h
    Uint16 dMvRange;                //062h
    Uint16 dDMvRange;               //063h
    Uint16 dMvMode;                 //064h
    Uint16 dIntComp;                //065h    ; dIntComp [0]
    Uint16 dLumScale;               //066h    ; dIntScale[0]
    Uint16 dLumShift;               //067h    ; dIntShift[0]
    Uint16 dIntComp1;               //068h    ; dIntComp [1]
    Uint16 dLumScale1;              //069h    ; dIntScale[1]
    Uint16 dLumShift1;              //06Ah    ; dIntShift[1]
    Uint16 dBitpCoded;              //06Bh
    Uint16 dCondOver;               //06Ch
    Uint16 dBitpMode0;              //06Dh
    Uint16 dBitpMode1;              //06Eh
    Uint16 dBitpMode2;              //06Fh
    Uint16 dMbModeTab;              //070h
    Uint16 dMv2BpTab;               //071h
    Uint16 dMv4BpTab;               //072h
    Uint16 dMvTab;                  //073h
    Uint16 dCbpTab;                 //074h
    Uint16 dTTMbf;                  //075h
    Uint16 dTTFrm;                  //076h
    Uint16 dTransDcTab;             //077h
    Uint16 dTransAcFrm0;            //078h
    Uint16 dTransAcFrm1;            //079h
    Uint16 dPQIndexLe8;             //07Ah
    Uint16 dPQuant0;                //07Bh
    Uint16 dPQuant1;                //07Ch
    Uint16 dHalfQp;                 //07Dh
    Uint16 dPQuantizer;             //07Eh
    Uint16 dDQuantFrm;              //07Fh
    Uint16 dDQProfile;              //080h
    Uint16 dDQEdge;                 //081h
    Uint16 dDQBiLevel;              //082h
    Uint16 dX8IntraMb;              //083h
    Uint16 dDctTabMb;               //084h
    Uint16 dPicType;                //085h
    Uint16 dMbAddr;                 //086h
    Uint16 dMbPosX;                 //087h
    Uint16 dMbPosY;                 //088h
    Uint16 reserved0;               //089h
    Uint16 reserved1;               //08Ah
    Uint16 reserved2;               //08Bh
} Coda9Vc1DirectMemory;

typedef struct {
    Uint16 tmp1;                    //010h
    Uint16 tmp2;                    //011h
    Uint16 tmp3;                    //012h

    Uint16 dPicX;                   //013h
    Uint16 dPicY;                   //014h
    Uint16 dProgressSeq;            //015h
    Uint16 dProfileId;              //016h
    Uint16 dLevelId;                //017h
    Uint16 dLowDelay;               //018h
    Uint16 dSeqChanged;             //019h
    Uint16 dPicType;                //01Ah
    Uint16 dFrameType;              //01Bh
    Uint16 dPictureDistance;        //01Ch
    Uint16 dProgressiveFrame;       //01Dh
    Uint16 dPictureStructure;       //01Eh
    Uint16 dTopFieldFirst;          //01Fh
    Uint16 dRepeatFirstField;       //020h
    Uint16 dFixedPictureQp;         //021h
    Uint16 dPicQp;                  //022h
    Uint16 dSkipModeFlag;           //023h
    Uint16 dDeblkOffSetA;           //024h
    Uint16 dDeblkOffSetB;           //025h
    Uint16 dPictureReferenceFlag;   //026h
    Uint16 dLoopFilterDisable;      //027h
    Uint16 dHeaderDecoded;          //028h
    Uint16 dTmpVal;                 //029h
    Uint16 dQpYLeft;                //02Ah
    Uint16 dQpYUpper;               //02Bh
    Uint16 dAvgQpLeft;              //02Ch
    Uint16 dAvgQpUpper;             //02Dh
    Uint16 dChromaQpCur;            //02Eh
    Uint16 dChromaQpLeft;           //02Fh
    Uint16 dFixedSliceQp;           //030h
    Uint16 dSliceQp;                //031h
    Uint16 dSliceAddress;           //032h
    Uint16 dFieldPic;               //033h
    Uint16 reserved[45];
    Uint16 dWeightQuantFlag;        //061h
    Uint16 dWeightQuantParam;       //062h
    Uint16 dWeightQuantModel;       //063h
    Uint16 dEntropyModeFlag;        //064h
    Uint16 dPskipRef;               //065h
    Uint16 dNewBdirect;             //066h
    Uint16 dQpDeltaCb;              //068h
    Uint16 dQpDeltaCr;              //069h
} Coda9AvsDirectMemory;

typedef struct {
    Uint16 pWqm_8x8[64];            //350h
} Coda9AvsIndirectMemory;
#pragma pack(pop)

#endif // SMB_CTRL_H_INCLUDED
