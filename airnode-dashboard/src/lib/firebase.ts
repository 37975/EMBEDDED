// Import the functions you need from the SDKs you need
import { initializeApp } from "firebase/app";
import { getAnalytics } from "firebase/analytics";
import { getDatabase } from "firebase/database";
// TODO: Add SDKs for Firebase products that you want to use
// https://firebase.google.com/docs/web/setup#available-libraries

// Your web app's Firebase configuration
// For Firebase JS SDK v7.20.0 and later, measurementId is optional
const firebaseConfig = {
  apiKey: "AIzaSyCoU3257zviNl_L0EyLSRHhW5bmXsnK_sk",
  authDomain: "embedded-cc90e.firebaseapp.com",
  databaseURL: "https://embedded-cc90e-default-rtdb.asia-southeast1.firebasedatabase.app",
  projectId: "embedded-cc90e",
  storageBucket: "embedded-cc90e.firebasestorage.app",
  messagingSenderId: "24395871863",
  appId: "1:24395871863:web:47b3d1ccda03a2b9947872",
  measurementId: "G-Y3Q3377X75"
};

// Initialize Firebase
const app = initializeApp(firebaseConfig);
export const analytics = getAnalytics(app);
export const db = getDatabase(app);